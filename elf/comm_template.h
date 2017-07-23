/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _COMM_TEMPLATE_H_
#define _COMM_TEMPLATE_H_

#include <queue>
#include <cassert>
#include <type_traits>
#include <vector>
#include <cstring>
#include <string>
#include <atomic>
#include <iostream>
#include <random>
#include <algorithm>

#include "pybind_helper.h"
#include "python_options_utils_cpp.h"

#include "state_collector.h"
#include "circular_queue.h"
#include "stats.h"

// Action/State communications between game simulators and game clients.
// SendData() add Key-Data pairs to the queue and WaitDataUntil() returns batched Key-Data pairs.
// Game clients write replies to some shared data structure,
// and call ReplyComplete() to indicate that the game simulators (who's waiting on WaitReply()) can continue.

// Game information
template <typename Data, typename Reply>
struct InfoT {
    // Meta info for this game.
    const MetaInfo *meta = nullptr;

    // Current sequence number.
    int seq = 0;    // seq * frame_skip == tick

    // How many games have been played.
    int game_counter = 0;

    // Hash code for current data.
    unsigned long hash_code = 0;

    // Game data (state, etc)
    Data data;

    // Model used to generate reply.
    int reply_version = 0;

    // Reply (action, etc)
    Reply reply;

    REGISTER_PYBIND_FIELDS(meta, seq, game_counter, hash_code, data, reply_version, reply);
};

template <typename T>
struct CondPerGroupT {
    int last_used_seq, last_seq;
    int game_counter;
    int freq_send;

    const int hist_overlap = 1;

    CondPerGroupT() : last_used_seq(0), last_seq(0), game_counter(0), freq_send(0) { }

    bool Check(int hist_len, T *data) {
        // Update game counter.
        int new_game_counter = data->newest().game_counter;
        if (new_game_counter > game_counter) {
            game_counter = new_game_counter;
            // Make sure no frame is missed. seq starts from 0.
            last_used_seq -= last_seq + 1;
        }
        int curr_seq = data->newest().seq;
        last_seq = curr_seq;
        if (data->size() < hist_len || curr_seq - last_used_seq < hist_len - hist_overlap) return false;
        last_used_seq = curr_seq;
        return true;
    }
};

template <typename In>
class CommT {
public:
    using Key = decltype(MetaInfo::query_id);
    using CollectorGroup = CollectorGroupT<In>;

private:
    struct Stat {
        Key key;
        int64_t freq;

        // Counter.
        std::unique_ptr<SemaCollector> counter;
        std::vector<CondPerGroupT<In>> conds;

        Stat(Key k) : key(k), freq(0) {
            counter.reset(new SemaCollector());
        }

        void InitCond(int ngroup) {
            for (int i = 0; i < ngroup; ++i) {
                conds.emplace_back();
            }
        }
    };

    struct GroupStat {
        std::vector<int> hist_lens;
        std::vector<int> subgroup;
    };

    ContextOptions _context_options;

    std::random_device _rd;
    std::mt19937 _g;

    std::vector<Key> _keys;
    std::vector<GroupStat> _exclusive_groups;

    std::vector<std::unique_ptr<CollectorGroup> > _groups;
    ctpl::thread_pool _pool;

    std::unique_ptr<SyncSignal> _signal;
    CommStats _stats;

    // Key -> Stat.
    std::unordered_map<Key, Stat> _map;

    bool _verbose;

    void compute_keys() {
        _keys.clear();
        for (int i = 0 ; i < _context_options.num_games; ++i) _keys.push_back(get_query_id(i, -1));
        // If multithread, register relevant keys.
        if (_context_options.max_num_threads) {
            for (int i = 0 ; i < _context_options.num_games; ++i) {
                for (int tid = 0; tid < _context_options.max_num_threads; ++tid) {
                    _keys.push_back(get_query_id(i, tid));
                }
            }
        }
    }

    void init_stats() {
        for (const Key& key : _keys) {
            _map.emplace(std::make_pair(key, Stat(key)));
        }
    }

public:
    CommT(const ContextOptions &context_options)
      : _context_options(context_options),  _g(_rd()), _verbose(context_options.verbose_comm) {
        _signal.reset(new SyncSignal());
        compute_keys();
        init_stats();
    }

    int GetT() const { return _context_options.T; }

    int AddCollectors(int batchsize, int hist_len, int exclusive_id) {
        _groups.emplace_back(new CollectorGroup(_groups.size(), _keys, batchsize, hist_len, _signal.get(), _context_options.verbose_collector));
        int gid = _groups.size() - 1;

        if ((int)_exclusive_groups.size() <= exclusive_id) {
            _exclusive_groups.emplace_back();
        }
        _exclusive_groups[exclusive_id].subgroup.push_back(gid);
        _exclusive_groups[exclusive_id].hist_lens.push_back(hist_len);

        return gid;
    }

    CollectorGroup &GetCollectorGroup(int gid) { return *_groups[gid]; }
    int num_groups() const { return _groups.size(); }

    void CollectorsReady() {
        if (_context_options.wait_per_group) {
            _signal->use_queue_per_group(_groups.size());
        }

        for (auto &p : _map) {
            p.second.InitCond(_exclusive_groups.size());
        }

        _pool.resize(_groups.size());
        for (auto &g : _groups) {
          CollectorGroup *p = g.get();
          _pool.push([p, this](int) { p->MainLoop(); });
        }
    }

    // Agent side.
    bool SendDataWaitReply(const Key& key, In& data) {
        auto it = _map.find(key);
        if (it == _map.end()) return false;
        Stat &stats = it->second;
        stats.freq ++;

        V_PRINT(_verbose, "[k=" << key << "] Start sending data, seq = " << data.newest().seq << " hist_size = " << data.size());
        // Send the key to all collectors in the container, if the key satisfy the gating function.
        std::vector<int> selected_groups;
        std::string str_selected_groups;

        // For each exclusive group, randomly select one.
        for (size_t i = 0; i < _exclusive_groups.size(); ++i) {
            const auto& subgroup = _exclusive_groups[i].subgroup;
            const auto& hist_lens = _exclusive_groups[i].hist_lens;
            int idx = _g() % subgroup.size();
            int gid = subgroup[idx];
            int hist_len = hist_lens[idx];

            if (stats.conds[i].Check(hist_len, &data)) {
                V_PRINT(_verbose, "[k=" << key << "] Pass test for group " << gid << " hist_len = " << hist_len);
                stats.conds[i].freq_send ++;

                _groups[gid]->SendData(key, &data);
                str_selected_groups += std::to_string(gid) + ",";
                selected_groups.push_back(gid);
            }
        }

        if (! selected_groups.empty()) {
            V_PRINT(_verbose, "[k=" << key << "] Sent to " << selected_groups.size() << " groups " << str_selected_groups << " waiting for the data to be processed");

            // Wait until all collectors have done their jobs.
            stats.counter->wait(selected_groups.size());

            V_PRINT(_verbose, "[k=" << key << "] All " << selected_groups.size() << " has done their jobs, Wait until the game is released");

            // Finally wait until resume is sent.
            for (const int gid : selected_groups) {
                _groups[gid]->WaitReply(key);
            }
        }

        V_PRINT(_verbose, "[k=" << key << "] Done with SendDataWaitReply");
        return true;
    }

    // Daemon side.
    Infos WaitBatchData(int time_usec = 0) { return _signal->wait_batch(-1, time_usec); }
    Infos WaitGroupBatchData(int group_id, int time_usec = 0) { return _signal->wait_batch(group_id, time_usec); }

    // Tell the collector that a reply was sent.
    bool Steps(const Infos& infos, int future_time_usec = 0) {
        // For invalid infos, return.
        if (infos.gid < 0) return false;

        std::vector<Key> keys = _groups[infos.gid]->GetBatchKeys();
        for (const Key &key : keys) {
            auto it = _map.find(key);
            if (it != _map.end()) {
                it->second.counter->notify();
            }
        }
        _groups[infos.gid]->SignalBatchUsed(future_time_usec);
        return true;
    }

    void PrintSummary() const {
        for (const auto &g : _groups) g->PrintSummary();
    }

    void PrepareStop() {
        for (const auto &g : _groups) g->SetBatchSize(1);
    }

    void Stop() {
        // Block all sends from game environments.
        Notif &done = _signal->GetDoneNotif();
        done.set();

        // Send a fake signal to deblock all threads.
        for (auto &g : _groups) g->NotifyAwake();

        // Wait until all threads are done.
        done.wait(_groups.size());
        _pool.stop();
    }
};

// Communication between main_loop and AI (which is in a separate thread).
// main_loop will send the environment data to AI, and call AI's Act().
// In Act(), AI will compute the best move and return it back.
//
template <typename Context>
class AICommT {
public:
    using Info = typename Context::Info;
    using HistType = CircularQueue<Info>;

    using AIComm = AICommT<Context>;

    using Comm = typename Context::Comm;
    using Reply = typename Context::Reply;
    using Data = typename Context::Data;

private:
    Comm *_comm;

    const MetaInfo _meta;

    CircularQueue<Info> _history;
    int _seq;
    int _game_counter;

    std::mt19937 _g;

    Info &curr() { return _history.ItemPush(); }
    const Info &curr() const { return _history.ItemPush(); }

public:
    AICommT(int id, Comm *comm)
        : _comm(comm), _meta(id), _history(comm->GetT()), _seq(0), _game_counter(0), _g(_meta.query_id) {
    }

    AICommT(const AIComm& parent, int child_id)
        : _comm(parent._comm), _meta(parent._meta, child_id), _history(parent._history),
          _seq(parent._seq), _game_counter(parent._game_counter), _g(_meta.query_id) {
    }

    void Prepare() {
        // we move the history forward.
        if (_history.full()) _history.Pop();
        curr().seq = _seq ++;
        curr().game_counter = _game_counter;
        curr().meta = &_meta;
    }

    const MetaInfo &GetMeta() const { return _meta; }

    Data *GetData() { return &curr().data; }
    std::mt19937 &gen() { return _g; }

    CircularQueue<Info> &history() { return _history; }
    const CircularQueue<Info> &history() const { return _history; }

    int seq() const { return _seq; }
    int game_counter() const { return _game_counter; }

    // Once we set all the information, send the state.
    bool SendDataWaitReply() {
        // Compute a hash code for data.
        // curr()->hash_code = serializer::hash_obj(curr()->data);
        /*
        if (curr().data.reward != 0.0) {
            std::cout << "[k=" << _meta.id << "][seq=" << curr().seq - 1 << "] "
                      << "Last_reward: " << curr().data.reward << std::endl;
        }
        */
        // Clear reply.
        curr().reply.Clear();
        curr().reply_version = 0;
        _history.Push();
        // std::cout << "[" << _meta.id << "] Before SendDataWaitReply" << std::endl;
        return _comm->SendDataWaitReply(_meta.query_id, _history);
        // std::cout << "[" << _meta.id << "] Done with SendDataWaitReply, continue" << std::endl;
    }

    // If we don't want to send the data to get reply, we can fill in by ourself.
    void FillInReply(const Reply &reply) {
        curr().reply = reply;
        _history.Push();
    }

    void Restart() {
        // The game terminates.
        _seq = 0;
        _game_counter ++;
    }

    // Spawn Child. The pointer will be own by the caller.
    AIComm *Spawn(int child_id) const {
        AIComm *child = new AIComm(*this, child_id);
        return child;
    }

    REGISTER_PYBIND;
};

// The game context, which could include multiple games.
template <typename _Options, typename _Data, typename _Reply>
class ContextT {
public:
    using Options = _Options;
    using Key = decltype(MetaInfo::query_id);
    using Reply = _Reply;
    using Data = _Data;

    using Info = InfoT<Data, Reply>;

    using Context = ContextT<Options, Data, Reply>;

    using HistType = CircularQueue<Info>;
    using DataAddr = DataAddrT<HistType>;
    using Comm = CommT<HistType>;

    using AIComm = AICommT<Context>;

    using State = _Data;
    using GameStartFunc =
      std::function<void (int game_idx, const Options& options, const std::atomic_bool &done, AIComm *)>;
    using CustomFieldFunc = typename DataAddr::CustomFieldFunc;

private:
    Comm _comm;
    std::vector<std::unique_ptr<AIComm>> _ai_comms;
    Options _options;
    ContextOptions _context_options;
    CustomFieldFunc _field_func;

    ctpl::thread_pool _pool;
    Notif _done;
    bool _game_started = false;

public:
    ContextT(const ContextOptions &context_options, const Options& options, CustomFieldFunc field_func = nullptr)
        : _comm(context_options), _options(options),
          _context_options(context_options), _field_func(field_func), _pool(context_options.num_games) {
    }

    int AddCollectors(int batchsize, int hist_len, int exclusive_id) {
        int gid = _comm.AddCollectors(batchsize, hist_len, exclusive_id);
        _comm.GetCollectorGroup(gid).GetDataAddr().RegCustomFunc(_field_func);
        return gid;
    }

    void Start(GameStartFunc game_start_func) {
        _comm.CollectorsReady();

        _ai_comms.resize(_pool.size());
        for (int i = 0; i < _pool.size(); ++i) {
            _ai_comms[i].reset(new AIComm{i, &_comm});
            _pool.push([i, this, &game_start_func](int){
                const std::atomic_bool &done = _done.flag();
                game_start_func(i, _options, done, _ai_comms[i].get());
                // std::cout << "G[" << i << "] is ending" << std::endl;
                _done.notify();
            });

            // TODO sleep to avoid random seed problem?
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        _game_started = true;
    }

    Infos Wait(int timeout_usec) { return _comm.WaitBatchData(timeout_usec); }
    Infos WaitGroup(int group_id, int timeout_usec) { return _comm.WaitGroupBatchData(group_id, timeout_usec); }
    void Steps(const Infos& infos) { _comm.Steps(infos); }

    const MetaInfo &meta(int i) const { return _ai_comms[i]->GetMeta(); }
    int size() const { return _ai_comms.size(); }

    DataAddr &GetDataAddr(int gid) {
        return _comm.GetCollectorGroup(gid).GetDataAddr();
    }

    void PrintSummary() const { _comm.PrintSummary(); }

    std::string Version() const {
#ifdef GIT_COMMIT_HASH
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
      return TOSTRING(GIT_COMMIT_HASH) "_" TOSTRING(GIT_UNSTAGED);
#else
      return "";
#endif
    }

    void Stop() {
        // Call the destructor.
        if (! _game_started) return;

        ctpl::thread_pool tmp_pool(1);
        const int wait_usec = 2;
        std::atomic_bool tmp_thread_done(false);
        tmp_pool.push([&](int) {
            while (! tmp_thread_done) {
                Steps(Wait(wait_usec));
            }
        });

        // First set all batchsize to be 1.
        _comm.PrepareStop();

        // Then stop all game threads.
        std::cout << "Stop all game threads ..." << std::endl;
        _done.set();
        _done.wait(_pool.size());
        _pool.stop();

        // Finally stop all collectors.
        std::cout << "Stop all collectors ..." << std::endl;
        _comm.Stop();

        tmp_thread_done = true;
        tmp_pool.stop();
        _game_started = false;
    }

    ~ContextT() {
        if (! _done.get()) Stop();
    }
};

#endif
