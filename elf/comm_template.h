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
#include <algorithm>

#include "pybind_helper.h"
#include "python_options_utils_cpp.h"

#include "state_collector.h"
#include "ai_comm.h"
#include "stats.h"

template <typename T>
struct CondPerGroupT {
    int last_used_seq, last_seq;
    int game_counter;
    int freq_send;

    const int hist_overlap = 1;

    CondPerGroupT() : last_used_seq(0), last_seq(0), game_counter(0), freq_send(0) { }

    bool Check(int hist_len, const T &info) {
        // Update game counter.
        int new_game_counter = info.data.newest().game_counter;
        if (new_game_counter > game_counter) {
            game_counter = new_game_counter;
            // Make sure no frame is missed. seq starts from 0.
            last_used_seq -= last_seq + 1;
        }
        int curr_seq = info.data.newest().seq;
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

        V_PRINT(_verbose, "[k=" << key << "] Start sending data, seq = " << info.data.newest().seq << " hist_len = " << info.data.size());
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

template <typename Key, typename Value>
const Value &get_value(const std::map<Key, Value>& dict, const Key &key, const Value &default_val) {
  auto it = dict.find(key);
  if (it == dict.end()) return default_val;
  else return it->second;
}

// The game context, which could include multiple games.
template <typename _Options, typename _Data>
class ContextT {
public:
    using Options = _Options;
    using Key = decltype(MetaInfo::query_id);

    using Data = _Data;
    using Context = ContextT<Options, Data>;
    using Info = InfoT<Data>;

    using Comm = CommT<Info>;
    using AIComm = AICommT<Comm>;

    using GameStartFunc =
      std::function<void (int game_idx, const Options& options, const std::atomic_bool &done, AIComm *)>;
    using EntryFunc =
      std::function<EntryInfo (const std::string &entry, const std::string &key, const std::string &v)>;

private:
    Comm _comm;
    std::vector<std::unique_ptr<AIComm>> _ai_comms;
    Options _options;
    ContextOptions _context_options;
    EntryFunc _entry_func;

    ctpl::thread_pool _pool;
    Notif _done;
    bool _game_started = false;

public:
    ContextT(const ContextOptions &context_options, const Options& options, EntryFunc entry_func)
        : _comm(context_options), _options(options), _context_options(context_options),
          _entry_func(entry_func), _pool(context_options.num_games) {
    }

    int AddCollectors(int batchsize, int hist_len, int exclusive_id) {
        int gid = _comm.AddCollectors(batchsize, hist_len, exclusive_id);
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

    std::vector<EntryInfo> GetTensorSpec(const std::string &key, const std::map<std::string, std::string> &desc) {
        std::vector<EntryInfo> entries;

        int batchsize = std::stoi(get_value<std::string, std::string>(desc, "_batchsize", "1"));
        int T = std::stoi(get_value<std::string, std::string>(desc, "_T", "1"));

        for (const auto &p : desc) {
            if (p.first.empty() || p.first[0] == '_') continue;

            EntryInfo entry_info = _entry_func(key, p.first, p.second);
            if (entry_info.sz.empty()) {
                std::cout << "[" << key << "][key=" << p.first << "][value=" << p.second << "]: key is not specified!" << std::endl;
                continue;
            }

            entry_info.SetBatchSizeAndHistory(batchsize, T);
            // std::cout << entry_info.PrintInfo() << std::endl << std::flush;
            entries.emplace_back(entry_info);
        }
        return entries;
    }

    void SetupTensor(int gid, const std::string &entry,
        const std::map<std::string, std::pair<std::uint64_t, std::size_t> > &pts) {
        _comm.GetCollectorGroup(gid).SetEntry(entry, pts);
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
