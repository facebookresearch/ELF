/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

//File: state_collector.h

#pragma once
#include <unordered_map>
#include <vector>
#include <iostream>

#include <memory>
#include <atomic>
#include <thread>
#include <sstream>

#include "pybind_helper.h"
#include "python_options_utils_cpp.h"

#include "ctpl_stl.h"
#include "data_addr.h"

#include "primitive.h"
#include "collector.hh"

struct Infos {
    int gid;
    int batchsize;

    Infos(int gid, int batchsize) : gid(gid), batchsize(batchsize) { }
    Infos() : gid(-1), batchsize(0) { }

    REGISTER_PYBIND_FIELDS(gid, batchsize);
};

class SyncSignal {
private:
    // A queue that contains the current queue of finished batch.
    CCQueue2<Infos> _queue;

    // Alternatively, we could also use a separate queue for each group.
    std::vector<CCQueue2<Infos>> _queue_per_group;

    // Whether we should terminate.
    Notif _done;

    // Lock for printing.
    std::mutex _mutex_cout;

public:
    SyncSignal() { }

    void use_queue_per_group(int num_groups) {
        _queue_per_group.resize(num_groups);
    }

    void push(int gid, int batchsize) {
        if (_queue_per_group.empty() || gid == -1) _queue.enqueue(Infos(gid, batchsize));
        else _queue_per_group[gid].enqueue(Infos(gid, batchsize));
    }

    // From the main thread.
    Infos wait_batch(int group_id, int time_usec = 0) {
        CCQueue2<Infos> *q = nullptr;
        if (group_id == -1 && _queue_per_group.empty()) q = &_queue;
        else if (group_id >= 0 && ! _queue_per_group.empty()) q = &_queue_per_group[group_id];
        else {
          throw std::range_error(
              "wait_batch error. group_id = " + std::to_string(group_id) +
              " while #queue_per_group = " + std::to_string(_queue_per_group.size()));
        }

        // Wait to check if there is any batch from any collectors.
        Infos infos;
        if (time_usec <= 0) {
            // pop_wait(*q, infos);
            q->wait_dequeue(infos);
        } else {
            // if (! pop_wait_time(*q, infos, time_usec)) infos.collector = nullptr;
            if (! q->wait_dequeue_timed(infos, time_usec)) infos.gid = -1;
        }
        return infos;
    }

    Notif &GetDoneNotif() { return _done; }

    // For sync printing.
    void Print(std::ostringstream &ss) {
        std::unique_lock<std::mutex> lock(_mutex_cout);
        std::cout << ss.rdbuf() << std::endl;
    }
    void Print(const std::string &s) {
        std::unique_lock<std::mutex> lock(_mutex_cout);
        std::cout << s << std::endl;
    }
};

#define PRINT(arg) { std::stringstream ss; ss << arg; _signal->Print(ss.str()); }
#define V_PRINT(verbose, arg) if (verbose) PRINT(arg)

template <typename T>
struct CollectConditionT {
    int last_seq, game_counter;
    int freq_send;

    const int hist_overlap = 1;

    CollectConditionT() : last_seq(-1), game_counter(0), freq_send(0) { }

    bool Check(int hist_len, T *data) {
        // Update game counter.
        if (data->game_counter() > game_counter) {
            game_counter = data->game_counter();
            last_seq = -1;
        }
        if (data->hist_size() < hist_len || data->seq() - last_seq < hist_len - hist_overlap) return false;
        last_seq = data->seq();
        return true;
    }
};

// Each collector group has a batch collector and a sequence of operators.
template <typename AIComm>
class CollectorGroupT {
public:
    using CollectCondition = CollectConditionT<AIComm>;
    using DataAddr = DataAddrT<AIComm>;
    using Key = decltype(MetaInfo::query_id);

private:
    const int _gid;
    // Here batchsize can be changed on demand.
    std::atomic<int> _batchsize;

    int _hist_len;

    // Current batch.
    std::vector<AIComm *> _batch;

    // Game conditions
    std::unordered_map<Key, CollectCondition> _conds;
    elf::BatchCollectorT<Key, AIComm> _batch_collector;

    DataAddr _data_addr;

    SyncSignal *_signal;

    bool _verbose;

    // Statistics
    int _num_enqueue;

    // Wakeup signal.
    Semaphore<int> _wakeup;

    void send_batch(int batchsize) {
        _wakeup.reset();
        _signal->push(_gid, batchsize);
    }

    int wait_batch_used() {
        int future_timeout;
        _wakeup.wait(&future_timeout);
        return future_timeout;
    }

public:
    CollectorGroupT(int gid, const std::vector<Key> &keys, int batchsize, int hist_len, 
            SyncSignal *signal, bool verbose)
        : _gid(gid), _batchsize(batchsize), _hist_len(hist_len), 
          _batch_collector(keys.size()), _signal(signal), _verbose(verbose) {
        for (const Key &key : keys) {
            _conds.emplace(std::make_pair(key, CollectCondition()));
        }
    }

    DataAddr &GetDataAddr() { return _data_addr; }
    int gid() const { return _gid; }

    void SetBatchSize(int batchsize) { _batchsize = batchsize; }

    // Game side.
    bool SendData(const Key &key, AIComm *data) {
        auto it = _conds.find(key);
        if (it == _conds.end()) return false;

        if (it->second.Check(_hist_len, data)) {
            if (_verbose) std::cout << "[" << key << "][" << _gid << "] c.SendData ... " << std::endl;
            // Collect data for this condition.
            _batch_collector.sendData(key, data);
            _num_enqueue ++;
            it->second.freq_send ++;
            return true;
        }
        return false;
    }

    void WaitReply(const Key &key) {
        V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] WaitReply for k = " << key);
        _batch_collector.waitReply(key);
    }

    // Main Loop
    void MainLoop() {
        V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] Starting MainLoop of collector, hist_len = " << _hist_len << " batchsize = " << _batchsize);
        while (true) {
            // Wait until we have a complete batch.
            _batch = _batch_collector.waitBatch(_batchsize);

            // Time to leave the loop.
            if (_batch.size() == 1 && _batch[0] == nullptr) break;

            V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] Compute input. batchsize = " << _batch.size());

            _data_addr.GetInputs(_batch);

            // Signal.
            V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] Send_batch. batchsize = " << _batch.size());
            send_batch(_batch.size());

            V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] Wait until the batch is processed");
            // Wait until it is processed.
            wait_batch_used();

            V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] PutReplies()");
            _data_addr.PutReplies(_batch);

            // Finally make the game run again.
            V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] Resume games");
            for (AIComm *ai_comm : _batch) {
                const Key& key = ai_comm->GetMeta().query_id;
                V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] Resume signal sent to k = " << key);
                _batch_collector.signalReply(key);
            }

            V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] All resume signal sent, batchsize = " << _batch.size());
        }

        V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] Collector ends. Notify the upper level");
        _signal->GetDoneNotif().notify();
    }

    // Daemon side.
    std::vector<Key> GetBatchKeys() const {
        std::vector<Key> keys;
        for (const AIComm *ai_comm : _batch) {
            keys.push_back(ai_comm->GetMeta().query_id);
        }
        return keys;
    }

    void SignalBatchUsed(int future_timeout) { _wakeup.notify(future_timeout); }

    void PrintSummary() const {
        std::cout << "Group[" << _gid << "]: HistLen = " << _hist_len << std::endl;
        std::cout << "[" << _gid << "]: #Enqueue: " << _num_enqueue << std::endl;
        for (const auto& p : _conds) {
            std::cout << "[" << _gid << "][" << p.first << "]: #Send[" << p.second.freq_send << "/"
                      << (float)p.second.freq_send / _num_enqueue << "]" << std::endl;
        }
    }

    // For other thread.
    void NotifyAwake() {
        // Kick the collector out of the waiting state by sending fake samples.
        _batch_collector.sendData(0, nullptr);
    }
};
