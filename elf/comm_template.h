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

#include "state_collector.h"
#include "ctpl_stl.h"
#include "circular_queue.h"
#include "data_addr.h"

#include "pybind_helper.h"
#include "python_options_utils_cpp.h"

// Action/State communications between game simulators and game clients.
// SendData() add Key-Data paris to the queue and WaitDataUntil() returns batched Key-Data pairs.
// Game clients write replies to some shared data structure,
// and call ReplyComplete() to indicate that the game simulators (who's waiting on WaitReply()) can continue.

// Game information exchanged between Python and C
template <typename Data, typename Reply>
struct InfoT {
    // Meta info for this game.
    // Always
    // Current sequence number.
    int seq = 0;    // seq * frame_skip == tick

    // Hash code for current data.
    unsigned long hash_code = 0;

    // Game data (state, etc)
    Data data;

    // Model used to generate reply.
    int reply_version = 0;

    // Reply (action, etc)
    Reply reply;

    REGISTER_PYBIND_FIELDS(seq, hash_code, data, reply_version, reply);
};

class CommStats {
private:
    // std::mutex _mutex;
    std::atomic<int64_t> _total, _last_total, _min, _max, _sum, _count;
    std::uniform_real_distribution<> _dis;

public:
    CommStats() : _total(0), _last_total(0), _min(0), _max(0), _sum(0), _count(0), _dis(0.5, 1.0) { }

    template <typename Generator>
    void Feed(const int64_t &v, Generator &g) {
        // std::unique_lock<std::mutex> lk{_mutex};
        _total ++;

        if (_last_total <= _total - 1000) {
            // Restart the counting.
            _min = v;
            _max = v;
            _sum = v;
            _count = 1;
            _last_total = _total.load();
        } else {
            // Get min and max.
            if (_min > v) _min = v;
            if (_max < v) _max = v;
            _sum += v;
            _count ++;
        }

        float avg_value = ((float)_sum) / _count;
        float min_value = _min;
        float max_value = _max;
        // lk.release();

        // If we have very large min_max distance, then do a speed control.
        if (max_value - min_value > avg_value / 30) {
            float ratio = (v - min_value) / (max_value  - min_value);
            if (ratio > 0.5) {
                if (_dis(g) <= ratio) {
                   std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }

        /*
        float ratio_diff = it->second.freq / even_freq - 1.0;
        if (ratio_diff >= 0) {
            ratio_diff = std::min(ratio_diff, 1.0f);
            const bool deterministic = false;
            if (deterministic) {
                int wait = int(100 * ratio_diff * 10 + 0.5);  // if (dis(_g) < ratio_diff)
                std::this_thread::sleep_for(std::chrono::microseconds(wait));
            } else {
                std::uniform_real_distribution<> dis(0.0, 1.0);
                if (dis(_g) <= ratio_diff) {
                   std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }
        */
    }
};

// A CommT has N state collectors, each with a different gating function.
template <typename DataAddr, typename Value>
class CommT {
public:
    using Key = decltype(MetaInfo::query_id);
    using StateCollector = StateCollectorT<DataAddr>;
    using CollectorGroup = CollectorGroupT<DataAddr>;
    using SyncSignal = typename StateCollector::SyncSignal;
    using Infos = typename SyncSignal::Infos;
    using TaskSignal = typename SyncSignal::TaskSignal;
    using CustomFieldFunc = typename DataAddr::CustomFieldFunc;

private:
    struct Stat {
        int idx;
        int64_t freq;
        Stat(int i) : idx(i), freq(0) { }
    };

    ContextOptions _context_options;

    std::random_device _rd;
    std::mt19937 _g;

    // Collectors.
    std::vector<std::unique_ptr<CollectorGroup> > _groups;
    int _total_collectors;

    std::unique_ptr<SyncSignal> _signal;
    CommStats _stats;

    // Key -> index.
    std::unordered_map<Key, Stat> _map;
    bool _verbose;
    CustomFieldFunc _field_func;

    // First register query_key then add collectors.
    void register_query_key(const Key& key) {
        assert(_map.find(key) == _map.end());
        _map.insert(std::make_pair(key, Stat(_map.size())));
    }

public:
    CommT(const ContextOptions &context_options, CustomFieldFunc field_func)
      : _context_options(context_options), _g(_rd()), _total_collectors(0),
        _verbose(context_options.verbose_comm), _field_func(field_func) {
        // Initialize comm.
        for (int i = 0 ; i < _context_options.num_games; ++i) register_query_key(i);

        // If multithread, register relevant keys.
        if (_context_options.max_num_threads) {
            for (int i = 0 ; i < _context_options.num_games; ++i) {
                for (int tid = 0; tid < _context_options.max_num_threads; ++tid) {
                    register_query_key(get_query_id(i, tid));
                }
            }
        }
        _signal.reset(new SyncSignal(_map.size()));
    }

    int GetT() const { return _context_options.T; }

    int AddCollectors(int batchsize, int hist_len, int num_collectors) {
        _groups.emplace_back(
            new CollectorGroup(_total_collectors, _groups.size(), batchsize, hist_len, num_collectors,
                  _field_func, _signal.get(), _context_options.verbose_collector));
        _total_collectors += num_collectors;
        return _groups.size() - 1;
    }

    CollectorGroup &GetCollectorGroup(int gid) { return *_groups[gid]; }
    int num_groups() const { return _groups.size(); }

    void CollectorsReady() {
        if (_context_options.wait_per_group) {
            _signal->use_queue_per_group(_groups.size());
        }
    }

    // Agent side.
    bool SendDataWaitReply(const Key& key, Value& value) {
        if (_signal->GetDoneNotif().get()) {
            V_PRINT(_verbose, "[" << key << "] Enter done mode. ");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return false;
        }

        auto it = _map.find(key);
        if (it == _map.end()) return false;
        int idx = it->second.idx;
        it->second.freq ++;
        // _stats.Feed(it->second.freq, _g);
        //
        V_PRINT(_verbose, "[k=" << key << "] Start sending data ... idx = " << idx);
        // Send the key to all collectors in the container, if the key satisfy the gating function.
        int num_groups = 0;
        for (auto &g : _groups) {
            if (g->SendData(idx, value.hist_size(), value.game_counter(), value.seq())) num_groups ++;
        }

        V_PRINT(_verbose, "[k=" << key << "] Start collecting ... ");

        // Wait for all the collector ids.
        // std::cout << "[" << key << "] wait for all collector ... #collectors = " << data.num_collectors << std::endl;
        std::vector<int> batch_data(_groups.size());
        TaskSignal cmd;
        const int total_task = NUM_TASK_CMD * num_groups;
        for (int i = 0; i < total_task; ++i) {
            // Note that wait_and_reset need to be done at once (withint the same critical region).
            // Otherwise it might erase signal from other collectors.
            _signal->GetSignal(idx, &cmd);
            if (_verbose) {
                V_PRINT(_verbose, "[k=" << key << "] Get collector: " << std::hex << cmd.collector << std::dec << " id: " << cmd.collector->id() << " gid: " << cmd.collector->gid());
            }
            auto &c = *cmd.collector;
            int cid = c.id();
            int gid = c.gid();

            if (cmd.type == SELECTED_IN_BATCH) {
                V_PRINT(_verbose, "[k=" << key << ",c=" << cid  << ",g=" << gid << "] Wake from BatchSelect " << i << "/" << total_task);
                // Then we have the batch for a collector.
                batch_data[gid] = c.CopyToInput(idx, value);
                V_PRINT(_verbose, "[k=" << key << ",c=" << cid << ",g=" << gid << "] done with CopyToInput. " << i << "/" << total_task);
            } else {
                V_PRINT(_verbose, "[k=" << key << ",c=" << cid << ",g=" << gid << "] Reply arrived " << i << "/" << total_task);
                c.CopyToReply(batch_data[gid], value);
                V_PRINT(_verbose, "[k=" << key << ",c=" << cid << ",g=" << gid << "] Done with CopyToReply " << i << "/" << total_task);
            }
        }

        V_PRINT(_verbose, "[k=" << key << "] Done with SendDataWaitReply");

        return true;
    }

    // Daemon side.
    Infos WaitBatchData(int time_usec = 0) { return _signal->wait_batch(-1, time_usec); }
    Infos WaitGroupBatchData(int group_id, int time_usec = 0) { return _signal->wait_batch(group_id, time_usec); }

    // Tell the collector that a reply was sent.
    bool Steps(const Infos& infos, int future_timeout_usec = 0) {
        // The batch is invalid, skip.
        if (infos.collector == nullptr) return false;
        infos.collector->SignalBatchUsed(future_timeout_usec);
        return true;
    }

    void PrintSummary() const {
        for (const auto &g : _groups) g->PrintSummary();
    }

    void Stop() {
        Notif &done = _signal->GetDoneNotif();
        done.set();
        for (auto &g : _groups) g->NotifyAwake();
        const int timeout_usec = 2;

        if (_context_options.wait_per_group) {
          done.wait(_total_collectors, [this]() {
              int group_id = _g() % _groups.size();
              Steps(WaitGroupBatchData(group_id, timeout_usec));
          });
        } else {
          done.wait(_total_collectors, [this]() {
              Steps(WaitBatchData(timeout_usec));
          });
        }
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

    using Context = ContextT<Options, Data, Reply>;
    using AIComm = AICommT<Context>;
    using DataAddr = DataAddrT<AIComm>;

    using Comm = CommT<DataAddr, AIComm>;
    using Info = InfoT<Data, Reply>;
    using State = _Data;
    using GameStartFunc =
      std::function<void (int game_idx, const Options& options, const std::atomic_bool &done, AIComm *)>;
    using CustomFieldFunc = typename DataAddr::CustomFieldFunc;

    using Infos = typename Comm::Infos;

private:
    Comm _comm;
    std::vector<std::unique_ptr<AIComm>> _ai_comms;
    Options _options;
    ContextOptions _context_options;

    ctpl::thread_pool _pool;
    Notif _done;
    bool _game_started = false;

public:
    ContextT(const ContextOptions &context_options, const Options& options, CustomFieldFunc field_func = nullptr)
        : _comm(context_options, field_func), _options(options),
          _context_options(context_options), _pool(context_options.num_games) {
    }

    int AddCollectors(int batchsize, int hist_len, int num_collectors) {
        return _comm.AddCollectors(batchsize, hist_len, num_collectors);
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

    DataAddr &GetDataAddr(int gid, int id_within_group) {
        return _comm.GetCollectorGroup(gid).GetCollector(id_within_group).GetDataAddr();
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
        std::cout << "Beginning stop all collectors ..." << std::endl;
        _comm.Stop();
        std::cout << "Stop all game threads ..." << std::endl;
        if (_game_started) {
            _done.set();
            _done.wait(_pool.size());
            _pool.stop();
            _game_started = false;
        }
    }

    ~ContextT() {
        if (! _done.get()) Stop();
    }
};

#endif
