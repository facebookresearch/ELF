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

// Game information exchanged between Python and C
template <typename Data, typename Reply>
struct InfoT {
    // Meta info for this game.
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

    REGISTER_PYBIND_FIELDS(seq, game_counter, hash_code, data, reply_version, reply);
};

// A CommT has N state collectors, each with a different gating function.
template <typename AIComm>
class CommT {
public:
    using Key = decltype(MetaInfo::query_id);
    using CollectorGroup = CollectorGroupT<AIComm>;

private:
    struct Stat {
        Key key;
        int64_t freq;

        // Counter.
        std::unique_ptr<SemaCollector> counter;

        Stat(Key k) : key(k), freq(0) {
            counter.reset(new SemaCollector());
        }
    };

    ContextOptions _context_options;

    std::random_device _rd;
    std::mt19937 _g;

    int _num_keys;
    std::map<int, std::vector<int>> _exclusive_groups;

    std::vector<std::unique_ptr<CollectorGroup> > _groups;
    ctpl::thread_pool _pool;

    std::unique_ptr<SyncSignal> _signal;
    CommStats _stats;

    // Key -> Stat.
    std::unordered_map<Key, Stat> _map;

    bool _verbose;

    // First register query_key then add collectors.
    Stat &get_stats(const Key& key) {
        auto it = _map.find(key);
        if (it == _map.end()) {
            auto info = _map.emplace(std::make_pair(key, Stat(_map.size())));
            return info.first->second;
        } else return it->second;
    }

    std::vector<Key> get_keys() const {
      std::vector<Key> keys;
      for (int i = 0 ; i < _context_options.num_games; ++i) keys.push_back(get_query_id(i, -1));
      // If multithread, register relevant keys.
      if (_context_options.max_num_threads) {
        for (int i = 0 ; i < _context_options.num_games; ++i) {
          for (int tid = 0; tid < _context_options.max_num_threads; ++tid) {
            keys.push_back(get_query_id(i, tid));
          }
        }
      }
      return keys;
    }

public:
    CommT(const ContextOptions &context_options)
      : _context_options(context_options),  _g(_rd()), _verbose(context_options.verbose_comm) {
        _signal.reset(new SyncSignal());
        _num_keys = _context_options.num_games * (_context_options.max_num_threads + 1);
    }

    int GetT() const { return _context_options.T; }

    int AddCollectors(int batchsize, int hist_len, int exclusive_id) {
        _groups.emplace_back(new CollectorGroup(_groups.size(), _num_keys, batchsize, hist_len, _signal.get(), _context_options.verbose_collector));
        int gid = _groups.size() - 1;

        auto it = _exclusive_groups.find(exclusive_id);
        if (it == _exclusive_groups.end()) {
            it = _exclusive_groups.emplace(std::make_pair(exclusive_id, std::vector<int>())).first;
        }
        it->second.push_back(gid);

        return gid;
    }

    CollectorGroup &GetCollectorGroup(int gid) { return *_groups[gid]; }
    int num_groups() const { return _groups.size(); }

    void CollectorsReady() {
        if (_context_options.wait_per_group) {
            _signal->use_queue_per_group(_groups.size());
        }

        _pool.resize(_groups.size());
        for (auto &g : _groups) {
          CollectorGroup *p = g.get();
          _pool.push([p, this](int) { p->MainLoop(); });
        }
    }

    // Agent side.
    bool SendDataWaitReply(const Key& key, AIComm& ai_comm) {
        Stat &stats = get_stats(key);
        stats.freq ++;

        V_PRINT(_verbose, "[k=" << key << "] Start sending data");
        // Send the key to all collectors in the container, if the key satisfy the gating function.
        std::vector<int> selected_groups;
        std::string str_selected_groups;

        // For each exclusive group, randomly select one.
        for (const auto &p : _exclusive_groups) {
            int idx = _g() % p.second.size();
            int gid = p.second[idx];

            if (_groups[gid]->SendData(key, &ai_comm)) {
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
        std::vector<Key> keys = _groups[infos.gid]->GetBatchKeys();
        for (const Key &key : keys) {
            get_stats(key).counter->notify();
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
    using AIComm = AICommT<Context>;
    using Comm = typename Context::Comm;
    using Reply = typename Context::Reply;
    using Data = typename Context::Data;
    using Info = typename Context::Info;

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
    }

    const MetaInfo &GetMeta() const { return _meta; }

    Data *GetData() { return &curr().data; }
    std::mt19937 &gen() { return _g; }

    // Python interface.
    // oldest = 0, newest = _history.maxlen() - 1
    Info &oldest(int i = 0) { return _history.get_from_push(_history.maxlen() - i - 1); }
    const Info &oldest(int i = 0) const { return _history.get_from_push(_history.maxlen() - i - 1); }
    Info &newest(int i = 0) { return _history.get_from_push(i); }
    const Info &newest(int i = 0) const { return _history.get_from_push(i); }

    int hist_size() const { return _history.size(); }
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
        return _comm->SendDataWaitReply(_meta.query_id, *this);
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
    using AIComm = AICommT<Context>;

    using DataAddr = DataAddrT<AIComm>;
    using Comm = CommT<AIComm>;
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
        _game_started = false;
    }

    ~ContextT() {
        if (! _done.get()) Stop();
    }
};

#endif
