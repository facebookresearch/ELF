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
#include <utility>

#include "pybind_helper.h"
#include "python_options_utils_cpp.h"

#include "state_collector.h"
#include "circular_queue.h"
#include "stats.h"

// Action/State communications between game simulators and game clients.
// SendData() add Key-Data pairs to the queue and WaitDataUntil() returns batched Key-Data pairs.
// Game clients write replies to some shared data structure,
// and call ReplyComplete() to indicate that the game simulators (who's waiting on WaitReply()) can continue.
//
struct SeqInfo {
    int seq;
    int game_counter;
    SeqInfo() : seq(0), game_counter(0) { }
    bool last_terminal() const {
        return seq == 0 && game_counter > 0;
    }
    void Inc() { seq ++; }
    void NewEpisode() { seq = 0; game_counter ++; }
};

// Game information
template <typename _Data>
struct InfoT {
    using Data = _Data;

    // Meta info for this game.
    const MetaInfo meta;

    // Game data (state, etc)
    // Note that data should have the following methods
    // Prepare(SeqInfo), initialize everything.
    // DonePrepare()
    Data data;

    InfoT(int id) : meta(id) { }
    InfoT(const InfoT<Data> &parent, int child_id) : meta(parent.meta, child_id) { }
};

template <typename T>
struct CondPerGroupT {
    int last_used_seq, last_seq;
    int game_counter;
    int freq_send;

    const int hist_overlap = 1;

    CondPerGroupT() : last_used_seq(0), last_seq(0), game_counter(0), freq_send(0) { }

    bool Check(int hist_len, const T &info) {
        // Update game counter.
        int new_game_counter = info.data.newest().seq.game_counter;
        if (new_game_counter > game_counter) {
            game_counter = new_game_counter;
            // Make sure no frame is missed. seq starts from 0.
            last_used_seq -= last_seq + 1;
        }
        int curr_seq = info.data.newest().seq.seq;
        last_seq = curr_seq;
        if (info.data.size() < hist_len || curr_seq - last_used_seq < hist_len - hist_overlap) return false;
        last_used_seq = curr_seq;
        return true;
    }
};

template <typename In>
class CommT {
public:
    using Key = decltype(MetaInfo::query_id);
    using Data = typename In::Data;
    using Info = In;
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
    bool SendDataWaitReply(const Key& key, In& info) {
        auto it = _map.find(key);
        if (it == _map.end()) return false;
        Stat &stats = it->second;
        stats.freq ++;

        V_PRINT(_verbose, "[k=" << key << "] Start sending data, seq = " << info.data.newest().seq.seq << " hist_len = " << info.data.size());
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

            if (stats.conds[i].Check(hist_len, info)) {
                V_PRINT(_verbose, "[k=" << key << "] Pass test for group " << gid << " hist_len = " << hist_len);
                stats.conds[i].freq_send ++;

                _groups[gid]->SendData(key, &info);
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

template <typename _Data>
class HistT {
private:
    using Data = _Data;
    using DataPrepareReturn = decltype(std::declval<Data>().Prepare(SeqInfo()));
    std::unique_ptr<CircularQueue<Data>> _h;

public:
    void InitHist(int len) {
        _h.reset(new CircularQueue<Data>(len));
    }

    DataPrepareReturn Prepare(const SeqInfo &seq) {
        // we move the history forward.
        if (_h->full()) _h->Pop();
        _h->Push();
        return newest().Prepare(seq);
    }

    // Note that these two will be called after .Push, so we need to retrieve them from .newest().
    // TODO: Make them consistent.
    int size() const { return _h->size(); }

    // oldest = 0, newest = _h->maxlen() - 1
    Data &oldest(int i = 0) { return _h->get_from_push(_h->maxlen() - i - 1); }
    const Data &oldest(int i = 0) const { return _h->get_from_push(_h->maxlen() - i - 1); }
    Data &newest(int i = 0) { return _h->get_from_push(i); }
    const Data &newest(int i = 0) const { return _h->get_from_push(i); }
};

// Communication between main_loop and AI (which is in a separate thread).
// main_loop will send the environment data to AI, and call AI's Act().
// In Act(), AI will compute the best move and return it back.
//
template <typename _Comm>
class AICommT {
public:
    using Comm = _Comm;
    using AIComm = AICommT<Comm>;
    using Data = typename Comm::Data;
    using Info = typename Comm::Info;
    using DataPrepareReturn = decltype(std::declval<Data>().Prepare(SeqInfo()));

private:
    Comm *_comm;

    Info _info;
    SeqInfo _curr_seq;

    std::mt19937 _g;

public:
    AICommT(int id, Comm *comm) : _comm(comm), _info(id), _g(_info.meta.query_id) {
        // TODO Make _data constructor more generalizable.
        _info.data.InitHist(comm->GetT());
    }

    AICommT(const AIComm& parent, int child_id)
        : _comm(parent._comm), _info(parent._info, child_id), _curr_seq(parent._curr_seq), _g(_info.meta.query_id) {
    }

    DataPrepareReturn Prepare() {
        // we move the history forward.
        DataPrepareReturn ret = _info.data.Prepare(_curr_seq);
        _curr_seq.Inc();
        return ret;
    }

    const Info &info() const { return _info; }
    Info &info() { return _info; }

    std::mt19937 &gen() { return _g; }

    const SeqInfo &seq_info() const { return _curr_seq; }

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
        // std::cout << "[" << _meta.id << "] Before SendDataWaitReply" << std::endl;
        return _comm->SendDataWaitReply(_info.meta.query_id, _info);
        // std::cout << "[" << _meta.id << "] Done with SendDataWaitReply, continue" << std::endl;
    }

    void Restart() {
        _curr_seq.NewEpisode();
    }

    // Spawn Child. The pointer will be own by the caller.
    AIComm *Spawn(int child_id) const {
        AIComm *child = new AIComm(*this, child_id);
        return child;
    }

    REGISTER_PYBIND;
};

// The game context, which could include multiple games.
template <typename _Options, typename _Data>
class ContextT {
public:
    using Options = _Options;
    using Key = decltype(MetaInfo::query_id);

    using Data = _Data;
    using Info = InfoT<Data>;

    using Context = ContextT<Options, Data>;
    using DataAddr = DataAddrT<Info>;
    using Comm = CommT<Info>;
    using AIComm = AICommT<Comm>;

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

    const MetaInfo &meta(int i) const { return _ai_comms[i]->info().meta; }
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
