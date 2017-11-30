/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include <queue>
#include <cassert>
#include <type_traits>
#include <vector>
#include <cstring>
#include <string>
#include <atomic>
#include <iostream>
#include <random>
#include <map>
#include <algorithm>

#include "pybind_helper.h"
#include "python_options_utils_cpp.h"

#include "state_collector.h"
#include "ai_comm.h"
#include "stats.h"
#include "member_check.h"
#include "signal.h"

struct GroupStat {
    int gid;
    int hist_len;
    std::string name;

    GroupStat() : gid(-1), hist_len(1) { }
    std::string info() const {
        return "[gid=" + std::to_string(gid) + "][T=" + std::to_string(hist_len) + "][name=\"" + name + "\"]";
    }

    // Note that gid will be set by C++ side.
    REGISTER_PYBIND_FIELDS(hist_len, name);
};

#define ADD_COND_CHECK(field_name) \
    MEMBER_CHECK(field_name);\
    template <typename S_ = typename T::State, typename std::enable_if<has_##field_name<S_>::value>::type *U = nullptr> \
    bool check_##field_name(const GroupStat &gstat, const S_ &record) {\
        if (! gstat.field_name.empty() && gstat.field_name != record.field_name) return false;\
        return true;\
    }\
    template <typename S_ = typename T::State, typename std::enable_if<! has_##field_name<S_>::value>::type *U = nullptr>\
    bool check_##field_name(const GroupStat &, const S_ &) { return true; }

template <typename T>
struct CondPerGroupT {
    int last_used_seq, last_seq;
    int game_counter;
    int freq_send;

    const int hist_overlap = 1;

    CondPerGroupT() : last_used_seq(0), last_seq(0), game_counter(0), freq_send(0) { }

    ADD_COND_CHECK(name)

    bool Check(const GroupStat &gstat, const T &info) {
        // Check whether this record is even relevant.
        // If we have specified player id and the player id from the info is irrelevant
        // from what is specified, then we skip.
        const auto &record = info.data.newest();
        if (! check_name(gstat, record)) return false;

        // std::cout << "Check " << gstat.info() << " record.name = " << record.name << std::endl;

        // Update game counter.
        int new_game_counter = record.game_counter;
        if (new_game_counter > game_counter) {
            game_counter = new_game_counter;
            // Make sure no frame is missed. seq starts from 0.
            last_used_seq -= last_seq + 1;
        }
        int curr_seq = record.seq;
        last_seq = curr_seq;

        // Check whether we want to put this record in.
        if (info.data.size() < gstat.hist_len || curr_seq - last_used_seq < gstat.hist_len - hist_overlap) return false;
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
    using Infos = InfosT<Data>;
    using SyncSignal = SyncSignalT<Data>;
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

    ContextOptions _context_options;

    std::random_device _rd;
    std::mt19937 _g;

    std::vector<Key> _keys;
    std::vector<std::vector<GroupStat>> _exclusive_groups;

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

    int AddCollectors(int batchsize, int exclusive_id, int timeout_usec, const GroupStat &gstat) {
        _groups.emplace_back(new CollectorGroup(_groups.size(), _keys, batchsize, _signal.get(),
                    _context_options.verbose_collector, timeout_usec));
        int gid = _groups.size() - 1;

        if ((int)_exclusive_groups.size() <= exclusive_id) {
            _exclusive_groups.emplace_back();
        }
        _exclusive_groups[exclusive_id].push_back(gstat);
        _exclusive_groups[exclusive_id].back().gid = gid;
        return gid;
    }

    std::string GetCollectorInfos() const {
        std::stringstream ss;
        for (size_t i = 0; i < _exclusive_groups.size(); ++i) {
            ss << "Group " << i << ": " << std::endl;
            for (size_t idx = 0; idx < _exclusive_groups[i].size(); ++idx) {
                const GroupStat &gstat = _exclusive_groups[i][idx];
                ss << "  " << _groups[gstat.gid]->info() << " Info: " << gstat.info() << std::endl;
            }
        }
        return ss.str();
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
        if (it == _map.end()) {
            V_PRINT(_verbose, "[k=" << key << "] seq = " << info.data.newest().seq << " hist_len = " << info.data.size() << ", key[" << key << "] invalid! ");
            return false;
        }
        Stat &stats = it->second;
        stats.freq ++;

        V_PRINT(_verbose, "[k=" << key << "] Start sending data, seq = " << info.data.newest().seq << " hist_len = " << info.data.size());
        // Send the key to all collectors in the container, if the key satisfy the gating function.
        std::vector<int> selected_groups;
        std::string str_selected_groups;

        // For each exclusive group, randomly select one.
        for (size_t i = 0; i < _exclusive_groups.size(); ++i) {
            int idx = _g() % _exclusive_groups[i].size();
            const GroupStat &gstat = _exclusive_groups[i][idx];

            if (stats.conds[i].Check(gstat, info)) {
                V_PRINT(_verbose, "[k=" << key << "] Pass test for group " << gstat.gid << " name = " << gstat.name << " hist_len = " << gstat.hist_len);
                stats.conds[i].freq_send ++;

                _groups[gstat.gid]->SendData(key, &info);
                str_selected_groups += std::to_string(gstat.gid) + ",";
                selected_groups.push_back(gstat.gid);
            }
        }

        if (! selected_groups.empty()) {
            V_PRINT(_verbose, "[k=" << key << "] Sent to " << selected_groups.size() << " groups " << str_selected_groups << " waiting for the data to be processed");

            // Wait until all collectors have done their jobs.
            stats.counter->wait(selected_groups.size());
            stats.counter->reset();

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

// The game context, which could include multiple games.
template <typename _Options, typename _Data>
class ContextT {
public:
    using Options = _Options;
    using Key = decltype(MetaInfo::query_id);

    using Data = _Data;
    using Infos = InfosT<Data>;
    using State = typename Data::State;
    using Context = ContextT<Options, Data>;
    using Info = InfoT<Data>;

    using Comm = CommT<Info>;
    using AIComm = AICommT<Comm>;

    using GameStartFunc = std::function<void (int game_idx, const ContextOptions &context_options, const Options& options, const elf::Signal &signal, Comm *comm)>;

private:
    Comm _comm;
    Options _options;
    ContextOptions _context_options;

    ctpl::thread_pool _pool;
    Notif _done;
    std::atomic_bool _prepare_stop;
    bool _game_started = false;

public:
    ContextT(const ContextOptions &context_options, const Options& options)
        : _comm(context_options), _options(options), _context_options(context_options),
          _pool(context_options.num_games) {
    }

    Comm &comm() { return _comm; }
    const Comm &comm() const { return _comm; }

    const ContextOptions &context_options() const { return _context_options; }
    const Options &options() const { return _options; }

    void Start(GameStartFunc game_start_func) {
        _comm.CollectorsReady();

        // Now we start all jobs.
        for (int i = 0; i < _pool.size(); ++i) {
            _pool.push([i, this, &game_start_func](int){
                elf::Signal signal(_done.flag(), _prepare_stop);
                game_start_func(i, _context_options, _options, signal, &_comm);
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

    int size() const { return _pool.size(); }

    void PrintSummary() const { _comm.PrintSummary(); }

    std::string Version() const {
#ifdef GIT_COMMIT_HASH
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
      return TOSTRING(GIT_COMMIT_HASH) "_" TOSTRING(GIT_UNSTAGED);
#else
      return "";
#endif
#undef STRINGIFY
#undef TOSTRING
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

        _prepare_stop = true;

        // First set all batchsize to be 1.
        std::cout << "Prepare to stop ..." << std::endl;
        _comm.PrepareStop();

        // Then stop all game threads.
        std::cout << "Stop all game threads ..." << std::endl;
        _done.set();
        _done.wait(_pool.size());
        _pool.stop();

        // Finally stop all collectors.
        std::cout << "Stop all collectors ..." << std::endl;
        _comm.Stop();

        std::cout << "Stop tmp pool..." << std::endl;
        tmp_thread_done = true;
        tmp_pool.stop();
        _game_started = false;
    }

    ~ContextT() {
        if (! _done.get()) Stop();
    }
};
