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
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include <sstream>

#include "blockingconcurrentqueue.h"
#include "pybind_helper.h"
#include "ctpl_stl.h"

template <typename T>
using CCQueue2 = moodycamel::BlockingConcurrentQueue<T>;

#ifdef USE_TBB
  #include <tbb/concurrent_queue.h>

  template <typename T>
  using CCQueue = tbb::concurrent_queue<T>;

  template <typename T>
  void push_q(CCQueue<T>& q, const T &val) {
     q.push(val);
  }

  template <typename T>
  void pop_wait(CCQueue<T> &q, T &val) {
    while (true) {
      if (q.try_pop(val)) break;
    }
  }

  template <typename T>
  bool pop_wait_time(CCQueue<T> &q, T &val, int time_usec) {
    if (! q.try_pop(val)) {
      std::this_thread::sleep_for(std::chrono::microseconds(time_usec));
      if (! q.try_pop(val)) return false;
    }
    return true;
  }
#else
  template <typename T>
  using CCQueue = moodycamel::BlockingConcurrentQueue<T>;

  template <typename T>
  void push_q(CCQueue<T>& q, const T &val) {
     q.enqueue(val);
  }

  template <typename T>
  void pop_wait(CCQueue<T> &q, T &val) {
     q.wait_dequeue(val);
  }

  template <typename T>
  bool pop_wait_time(CCQueue<T> &q, T &val, int time_usec) {
     return q.wait_dequeue_timed(val, time_usec);
  }
#endif

class SemaCollector {
private:
    int _count;
    std::mutex _mutex;
    std::condition_variable _cv;

public:
    SemaCollector() : _count(0) { }

    inline void notify() {
        std::unique_lock<std::mutex> lock(_mutex);
        _count ++;
        _cv.notify_one();
    }

    inline int wait(int expected_count, int usec = 0) {
        std::unique_lock<std::mutex> lock(_mutex);
        if (usec == 0) {
            _cv.wait(lock, [this, expected_count]() { return _count >= expected_count; });
        } else {
            _cv.wait_for(lock, std::chrono::microseconds(usec),
                [this, expected_count]() { return _count >= expected_count; });
        }
        return _count;
    }

    inline void reset() {
        std::unique_lock<std::mutex> lock(_mutex);
        _count = 0;
        _cv.notify_all();
    }
};

class Notif {
private:
    std::atomic_bool _flag;   // flag to indicate stop
    SemaCollector _counter;

public:
    Notif() : _flag(false) { }
    const std::atomic_bool &flag() const { return _flag; }
    bool get() const { return _flag.load(); }
    void notify() { _counter.notify(); }

    void set() { _flag = true; }
    void wait(int n, std::function<void ()> f = nullptr) {
        _flag = true;
        if (f == nullptr) _counter.wait(n);
        else {
            while (true) {
              int current_cnt = _counter.wait(n, 10);
              // std::cout << "current cnt = " << current_cnt << " n = " << n << std::endl;
              if (current_cnt >= n) break;
              f();
            }
        }
    }
    void reset() {
        _counter.reset();
        _flag = false;
    }
};

template <typename T>
class Semaphore {
private:
    bool _flag;
    T _val;
    std::mutex _mutex;
    std::condition_variable _cv;

    inline void _raw_wait(std::unique_lock<std::mutex> &lock, int usec) {
        if (usec == 0) {
            _cv.wait(lock, [this]() { return _flag; });
        } else {
            _cv.wait_for(lock, std::chrono::microseconds(usec),  [this]() { return _flag; });
        }
    }

public:
    Semaphore() : _flag(false) { }

    inline void notify(T val) {
        std::unique_lock<std::mutex> lock(_mutex);
        _flag = true;
        _val = val;
        _cv.notify_one();
    }

    inline bool wait(T *val, int usec = 0) {
        std::unique_lock<std::mutex> lock(_mutex);
        _raw_wait(lock, usec);
        if (_flag) *val = _val;
        return _flag;
    }

    inline bool wait_and_reset(T *val, int usec = 0) {
        std::unique_lock<std::mutex> lock(_mutex);
        _raw_wait(lock, usec);
        bool last_flag = _flag;
        if (_flag) *val = _val;
        _flag = false;
        return last_flag;
    }

    inline void reset() {
        std::unique_lock<std::mutex> lock(_mutex);
        _flag = false;
    }
};

// DataAddr has a few methods.
// DataAddr::GetInput(const Value &v);
// DataAddr::PutReply(Value &v) const;

template <typename DataAddr>
class BatchExchangeT {
private:
    std::mutex _batch_mutex;
    std::vector<int> _batch_data;

    DataAddr _base;
    SemaCollector _sema_input, _sema_reply;

public:
    DataAddr &GetBase() { return _base; }

    // Each AICommT should call this once and only once.
    int AddBatch(int idx) {
        std::unique_lock<std::mutex> lk_batch{_batch_mutex};
        int batch_idx = _batch_data.size();
        _batch_data.emplace_back(idx);
        // std::cout << "After emplace_back. idx = " << idx << ", batch_idx = " << batch_idx << "#_batch_data = " << _batch_data.size() << std::endl;
        return batch_idx;
    }

    void InputReady() { _sema_input.notify(); }
    void ReplySaved() { _sema_reply.notify(); }

    // Daemon side.
    void Reset() {
        _sema_input.reset();
        _sema_reply.reset();
        _batch_data.clear();
    }

    const std::vector<int> &batch() const { return _batch_data; }

    void WaitUntilInputReady(int expected) { _sema_input.wait(expected); }
    void WaitUntilReplyDispatched() { _sema_reply.wait(_batch_data.size()); }
};

template <typename T>
struct InfosT {
    T *collector;
    int id;
    int id_in_group;
    int gid;
    int batchsize;

    InfosT(T* collector, int batchsize) : collector(collector), batchsize(batchsize) {
        if (collector != nullptr) {
            id = collector->id();
            id_in_group = collector->id_in_group();
            gid = collector->gid();
        }
    }
    InfosT() : collector(nullptr), id(-1), gid(-1), batchsize(0) { }

    REGISTER_PYBIND_FIELDS(id, gid, id_in_group, batchsize);
};

enum TaskType { SELECTED_IN_BATCH = 0, REPLY_ARRIVED, NUM_TASK_CMD };

template <typename T>
struct TaskSignalT {
    T *collector;
    TaskType type;
    TaskSignalT() : collector(nullptr), type(NUM_TASK_CMD) { }
    TaskSignalT(TaskType type, T* collector) : collector(collector), type(type) { }
};

template <typename T>
struct TaskDataT {
    // #Collectors interested in this sample.
    CCQueue2<TaskSignalT<T>> cmd_q;
};

template <typename T>
class SyncSignalT {
public:
    using Infos = InfosT<T>;
    using TaskSignal = TaskSignalT<T>;
    using TaskData = TaskDataT<T>;

private:
    // A queue that contains the current queue of finished batch.
    CCQueue2<InfosT<T>> _queue;

    // Alternatively, we could also use a separate queue for each group.
    std::vector<CCQueue2<InfosT<T>>> _queue_per_group;

    // For each query, there is a TaskData for its synchronization.
    std::unique_ptr<std::vector<TaskData>> _data;

    // Whether we should terminate.
    Notif _done;

    // Lock for printing.
    std::mutex _mutex_cout;

public:
    SyncSignalT(int num_games) {
        _data.reset(new std::vector<TaskData>(num_games));
    }

    void use_queue_per_group(int num_groups) {
        _queue_per_group.resize(num_groups);
    }

    void push(T *target, int gid, int batchsize) {
        if (_queue_per_group.empty() || gid == -1) _queue.enqueue(Infos(target, batchsize));
        else _queue_per_group[gid].enqueue(Infos(target, batchsize));
    }

    // From the main thread.
    Infos wait_batch(int group_id, int time_usec = 0) {
        CCQueue2<InfosT<T>> *q = nullptr;
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
            if (! q->wait_dequeue_timed(infos, time_usec)) infos.collector = nullptr;
        }
        if (infos.collector == nullptr) infos.id = infos.gid = infos.id_in_group = -1;
        return infos;
    }

    // For sender.
    void select_in_batch(T *target, int idx) {
        TaskData &data = _data->at(idx);
        data.cmd_q.enqueue(TaskSignal(SELECTED_IN_BATCH, target));
    }

    void reply_arrived(T *target, int idx) {
        TaskData &data = _data->at(idx);
        data.cmd_q.enqueue(TaskSignal(REPLY_ARRIVED, target));
    }

    void GetSignal(int idx, TaskSignal *cmd) {
        auto& data = _data->at(idx);
        data.cmd_q.wait_dequeue(*cmd);
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

    int num_games() const { return _data->size(); }
};

#define PRINT(arg) { std::stringstream ss; ss << arg; _signal->Print(ss.str()); }
#define V_PRINT(verbose, arg) if (verbose) PRINT(arg)

template <typename DataAddr>
class StateCollectorT {
public:
    using BatchExchange = BatchExchangeT<DataAddr>;
    using StateCollector = StateCollectorT<DataAddr>;
    using SyncSignal = SyncSignalT<StateCollector>;
    using CustomFieldFunc = typename DataAddr::CustomFieldFunc;

private:
    const int _id, _id_in_group, _gid;
    int _batchsize;
    SyncSignal *_signal;

    BatchExchange _exchange;
    CCQueue<int> Q;

    Semaphore<int> _wakeup;

    bool _verbose;
    int _num_enqueue, _num_wait, _num_steps;
    std::vector<int> _freq_send, _freq_steps;

    void send_batch(int batchsize) {
        _wakeup.reset();
        _signal->push(this, _gid, batchsize);
    }
    int wait_batch_used() {
        int future_timeout;
        _wakeup.wait(&future_timeout);
        return future_timeout;
    }

public:
    StateCollectorT(int id, int id_in_group, int gid, int batchsize, CustomFieldFunc field_func,
        SyncSignal *signal, bool verbose = false)
        : _id(id), _id_in_group(id_in_group), _gid(gid), _batchsize{batchsize}, _signal(signal),
          _verbose(verbose), _num_wait(0),
          _num_steps(0), _freq_send(signal->num_games(), 0), _freq_steps(signal->num_games(), 0) {
        _exchange.GetBase().RegCustomFunc(field_func);
    }

    // AICommT side.
    void SendData(int idx) {
        push_q(Q, idx);
        _num_enqueue ++;
        _freq_send[idx] ++;
    }

    template <typename Value>
    int CopyToInput(int idx, const Value& v) {
        int batch_idx = _exchange.AddBatch(idx);
        if (_verbose) _signal->Print("After AddBatch");
        _exchange.GetBase().GetInput(batch_idx, v);
        if (_verbose) _signal->Print("After GetInput");
        _exchange.InputReady();
        if (_verbose) _signal->Print("After InputReady");
        return batch_idx;
    }

    template <typename Value>
    void CopyToReply(int batch_idx, Value &v) {
        _exchange.GetBase().PutReply(batch_idx, v);
        _exchange.ReplySaved();
    }

    int id() const { return _id; }
    int gid() const { return _gid; }
    int id_in_group() const { return _id_in_group; }

    DataAddr &GetDataAddr() { return _exchange.GetBase(); }

    // Daemon Side.
    // no reentrable
    int WaitBatchData() {
        // It will skip all fake examples (but still count them)
        // This enables us to pull WaitBatchData() out of the waiting state by injecting fake samples.
        int batchsize = 0;
        for (int i = 0; i < _batchsize; ++i) {
            int k;
            pop_wait(Q, k);
            // Negative index means that it is a fake sample and we skip.
            if (k < 0) continue;

            // Once you get the idx, ask them to feed the data.

            V_PRINT(_verbose, "Get sample " << k << " cid = " << _id);
            _signal->select_in_batch(this, k);
            batchsize ++;
        }

        // Wait until all features are extracted.
        V_PRINT(_verbose, "[" << _id << "] Start WaitUntilInputReady");
        _exchange.WaitUntilInputReady(batchsize);
        V_PRINT(_verbose, "[" << _id << "] Done with WaitUntilInputReady");
        _num_wait ++;
        return batchsize;
    }

    int WaitBatchDataUntil(int timeout_usec) {
        // It will skip all fake examples (And does not count them)
        // This enables us to collect all samples, until batchsize = 0.
        std::int64_t timeout_usec_per_loop = timeout_usec / _batchsize;
        int batchsize = 0;
        while (batchsize < _batchsize) {
            int k;
            if (! pop_wait_time(Q, k, timeout_usec_per_loop)) break;
            if (k < 0) continue;
            _signal->select_in_batch(this, k);
            batchsize ++;
        }
        // Wait until all features are extracted.
        _exchange.WaitUntilInputReady(batchsize);
        _num_wait ++;
        return batchsize;
    }

    void Steps() {
        V_PRINT(_verbose, "#_exchange.batch() = " << _exchange.batch().size());
        for (int batch_idx : _exchange.batch()) {
            V_PRINT(_verbose, "Steps: idx = " << batch_idx << "#game = " << _signal->num_games());
            _freq_steps[batch_idx] ++;
            _signal->reply_arrived(this, batch_idx);
        }
        _num_steps += _exchange.batch().size();
        V_PRINT(_verbose, "[" << _id << "] Start WaitUntilReplyDispatched");
        _exchange.WaitUntilReplyDispatched();
        _exchange.Reset();
        V_PRINT(_verbose, "[" << _id << "] End WaitUntilReplyDispatched");
    }

    void SignalBatchUsed(int future_timeout) { _wakeup.notify(future_timeout); }

    // For other thread.
    void NotifyAwake() {
        // Kick the collector out of the waiting state by sending fake samples.
        for (int i = 0; i < 2 * _batchsize; ++i) push_q(Q, -1);
    }

    void MainLoop(int initial_timeout_usec) {
        int timeout_usec = initial_timeout_usec;
        while (true) {
            // Wait until we have a complete batch.
            int batchsize = (timeout_usec <= 0) ? WaitBatchData() : WaitBatchDataUntil(timeout_usec);

            V_PRINT(_verbose, "[" << _id << "] about to send batch: batchsize = " << batchsize);
            // Signal.
            send_batch(batchsize);

            V_PRINT(_verbose, "[" << _id << "] about to wait until the batch is processed");
            // Wait until it is processed.
            timeout_usec = wait_batch_used();

            V_PRINT(_verbose, "[" << _id << "] about to Step()");
            // Move forward.
            Steps();

            if (_signal->GetDoneNotif().get()) {
                // Switch to wait_until mode.
                timeout_usec = 1;
                V_PRINT(_verbose, "[" << _id << "] Exit mode: batch size = " << batchsize);
                // All sample drained, leave.
                if (batchsize == 0) break;
            }
        }

        V_PRINT(_verbose, "[" << _id << "] Collector ends. Notify the upper level");
        _signal->GetDoneNotif().notify();
    }

    void PrintSummary() const {
        std::cout << "[" << _id << "]: #Enqueue: " << _num_enqueue
                  << ", #Wait: " << _num_wait << ", #Steps: " << _num_steps << std::endl;
        for (size_t i = 0; i < _freq_send.size(); ++i) {
            std::cout << "[" << _id << "][" << i << "]: #Send[" << _freq_send[i] << "/"
                      << (float)_freq_send[i] / _num_enqueue << "],"
                      <<" #Steps[" << _freq_steps[i] <<"/" << (float)_freq_steps[i] / _num_steps << "]" << std::endl;
        }
    }
};

template <typename DataAddr>
class CollectorGroupT {
public:
    using StateCollector = StateCollectorT<DataAddr>;
    using SyncSignal = typename StateCollector::SyncSignal;
    using CustomFieldFunc = typename DataAddr::CustomFieldFunc;

private:
    std::vector<std::unique_ptr<StateCollector>> _collectors;
    const int _gid;
    int _hist_len;

    // Last seq number
    std::vector<int> _last_seq;
    // Last game counter.
    std::vector<int> _game_counter;

    // Random device.
    std::random_device _rd;
    std::mt19937 _g;

    ctpl::thread_pool _pool;
    bool _verbose;

public:
    CollectorGroupT(int start_id, int gid, int batchsize, int hist_len, int num_collectors,
        CustomFieldFunc field_func, SyncSignal *signal, bool verbose)
        : _gid(gid), _hist_len(hist_len), _last_seq(signal->num_games(), -1), _game_counter(signal->num_games(), 0),
          _g(_rd()), _pool(num_collectors), _verbose(verbose) {
        for (int i = 0; i < num_collectors; ++i) {
            _collectors.emplace_back(
                new StateCollector(start_id + i, i, gid, batchsize, field_func, signal, verbose));
            StateCollector *this_collector = _collectors.back().get();
            _pool.push([this_collector, this](int) { this_collector->MainLoop(0); });
        }
    }

    bool SendData(int idx, int curr_hist_len, int curr_game_counter, int curr_seq) {
        // Update game counter.
        if (curr_game_counter > _game_counter[idx]) {
            _game_counter[idx] = curr_game_counter;
            _last_seq[idx] = -1;
        }
        const int hist_overlap = 1;
        if (curr_hist_len < _hist_len || curr_seq - _last_seq[idx] < _hist_len - hist_overlap) return false;

        int collector_id = _g() % _collectors.size();
        if (_verbose) std::cout << "[" << idx << "][" << collector_id << "] c.SendData ... " << std::endl;
        _collectors[collector_id]->SendData(idx);

        _last_seq[idx] = curr_seq;

        return true;
    }

    void PrintSummary() const {
        std::cout << "Group[" << _gid << "]: HistLen = " << _hist_len << std::endl;
        for (const auto &c : _collectors) c->PrintSummary();
    }
    void NotifyAwake() {
        for (auto &c : _collectors) c->NotifyAwake();
    }

    int size() const { return _collectors.size(); }
    StateCollector &GetCollector(int i) { return *_collectors[i]; }
};
