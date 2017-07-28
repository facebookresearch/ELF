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

#include "primitive.h"
#include "collector.hh"
#include "hist.h"

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

struct EntryInfo {
  std::string key;
  std::string type;
  std::vector<int> sz;
  int extra_len;

  uint64_t p;
  std::size_t byte_size;

  EntryInfo() : p(0), byte_size(0) { }
  EntryInfo(const std::string& key, const std::string& type, std::initializer_list<int> l, int extra_len = 1)
    : key(key), type(type), sz(l), extra_len(extra_len), p(0), byte_size(0) {
  }

  void SetBatchSizeAndHistory(int batchsize, int T) {
      std::vector<int> sz2;
      sz2.push_back(batchsize);
      sz2.push_back(T);
      for (const int &v : sz) sz2.push_back(v);
      sz = sz2;
  }

  std::string PrintInfo() const {
      std::stringstream ss;
      ss << "[" << key << "][type=" << type << "]: ";
      for (const int &v : sz) {
        ss << v << ", ";
      }
      return ss.str();
  }

  REGISTER_PYBIND_FIELDS(key, type, sz, p, byte_size);
};

// Each collector group has a batch collector and a sequence of operators.
template <typename In>
class CollectorGroupT {
public:
    using Key = decltype(MetaInfo::query_id);
    using State = typename In::State;
    using Data = typename In::Data;
    using CopyItem = elf::CopyItemT<State>;
    using EntryFunc = std::function<EntryInfo (const std::string &key)>;

private:
    const int _gid;
    // Here batchsize can be changed on demand.
    int _batchsize;
    CCQueue2<int> _batchsize_q;
    Semaphore<int> _batchsize_back;

    int _hist_len;

    // Current batch.
    std::vector<In *> _batch;
    std::vector<Data *> _batch_data;
    elf::BatchCollectorT<Key, In> _batch_collector;

    std::vector<CopyItem> _copier_input;
    std::vector<CopyItem> _copier_reply;

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
          _batch_collector(keys), _signal(signal), _verbose(verbose) {
    }

    EntryInfo GetEntry(const std::string &key, EntryFunc entry_func) const {
        if (key.empty()) return EntryInfo();

        EntryInfo entry_info = entry_func(key);
        if (entry_info.sz.empty()) {
            std::cout << "[" << key << "] key is not specified!" << std::endl;
            return EntryInfo();
        }

        entry_info.SetBatchSizeAndHistory(_batchsize, _hist_len);
        return entry_info;
    }

    void AddEntry(const std::string &input_reply, const EntryInfo &e) {
        std::vector<CopyItem> *copier = nullptr;

        if (input_reply == "input") copier = &_copier_input;
        else if (input_reply == "reply") copier = &_copier_reply;
        else throw std::range_error("Unknown input_reply " + input_reply);

        auto *mm = State::get_mm(e.key);
        m_assert(mm != nullptr);
        copier->emplace_back(e.key, elf::SharedBuffer(e.p, e.byte_size), mm);
    }

    int gid() const { return _gid; }

    void SetBatchSize(int batchsize) {
        // std::cout << "Before send batchsize " << batchsize << std::endl;
        _batchsize_q.enqueue(batchsize);
        int dummy;
        // std::cout << "After send batchsize " << batchsize << " Waiting for reply" << std::endl;
        _batchsize_back.wait(&dummy);
        // std::cout << "Reply got. batchsize " << batchsize << std::endl;
    }

    // Game side.
    void SendData(const Key &key, In *data) {
        if (_verbose) std::cout << "[" << key << "][" << _gid << "] c.SendData ... " << std::endl;
        // Collect data for this condition.
        _batch_collector.sendData(key, data);
        _num_enqueue ++;
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
            int new_batchsize;
            if (_batchsize_q.wait_dequeue_timed(new_batchsize, 0)) {
                _batchsize = new_batchsize;
                // std::cout << "CollectorGroup: get new batchsize. batchsize = " << _batchsize << std::endl;
                _batchsize_back.notify(0);
                // std::cout << "CollectorGroup: After notification. batchsize = " << _batchsize << std::endl;
            }
            _batch = _batch_collector.waitBatch(_batchsize);
            _batch_data.clear();
            for (In *b : _batch) {
                _batch_data.push_back(&b->data);
            }

            // Time to leave the loop.
            if (_batch.size() == 1 && _batch[0] == nullptr) break;

            V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] Compute input. batchsize = " << _batch.size());

            elf::CopyToMem(_copier_input, _batch_data);

            // Signal.
            V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] Send_batch. batchsize = " << _batch.size());
            send_batch(_batch.size());

            V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] Wait until the batch is processed");
            // Wait until it is processed.
            wait_batch_used();

            V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] PutReplies()");
            elf::CopyFromMem(_copier_reply, _batch_data);

            // Finally make the game run again.
            V_PRINT(_verbose, "CollectorGroup: [" << _gid << "] Resume games");
            for (In *in : _batch) {
                const Key& key = in->meta.query_id;
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
        for (const In *in : _batch) {
            keys.push_back(in->meta.query_id);
        }
        return keys;
    }

    void SignalBatchUsed(int future_timeout) { _wakeup.notify(future_timeout); }

    void PrintSummary() const {
        /*
        std::cout << "Group[" << _gid << "]: HistLen = " << _hist_len << std::endl;
        std::cout << "[" << _gid << "]: #Enqueue: " << _num_enqueue << std::endl;
        for (const auto& p : _conds) {
            std::cout << "[" << _gid << "][" << p.first << "]: #Send[" << p.second.freq_send << "/"
                      << (float)p.second.freq_send / _num_enqueue << "]" << std::endl;
        }
        */
    }

    // For other thread.
    void NotifyAwake() {
        // Kick the collector out of the waiting state by sending fake samples.
        _batch_collector.sendData(0, nullptr);
    }
};
