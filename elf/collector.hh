/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

//File: collector.hh

#pragma once
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "lib/debugutils.hh"
#ifdef USE_TBB
#include <tbb/concurrent_queue.h>
#else
#include "blockingconcurrentqueue.h"
#endif

namespace elf {


template <typename Key, typename Value>
class CollectorWithCCQueue {
#ifdef USE_TBB
  tbb::concurrent_queue<int> Q;
#else
  moodycamel::BlockingConcurrentQueue<int> Q;
#endif

  std::unordered_map<Key, int> _index_map;

  struct TaskData {
    Value* val = nullptr;

    std::condition_variable cond;
    std::mutex mutex;
    std::atomic_bool flag{false};
    // reply has come or not
  };
  std::vector<std::unique_ptr<TaskData>> _data;

  int get_index(const Key &key) const {
    auto it = _index_map.find(key);
    if (it == _index_map.end()) {
        std::cout << "key [" << key << "] not found! this should never happen! " << std::endl;
        return -1;
    }   
    return it->second;
  }

  inline void notify(std::unique_ptr<TaskData>& d) {
    std::lock_guard<std::mutex> lg(d->mutex);
    d->flag.store(true);
    d->cond.notify_one();
  }

  public:
  explicit CollectorWithCCQueue(const std::vector<Key> &keys) {
      // Preload all the keys.
      for (size_t i = 0; i < keys.size(); ++i) {
          _data.emplace_back(new TaskData{});
          _index_map.emplace(keys[i], i);
      }
  }

  CollectorWithCCQueue(const CollectorWithCCQueue&) = delete;

  void sendData(const Key& key, Value* value) {
    int index = get_index(key);
    if (index < 0) throw std::range_error("[sendData] key " + std::to_string(key) + " not found!");

    _data[index]->val = value;
#ifdef USE_TBB
    Q.push(index);
#else
    Q.enqueue(index);
#endif
  }

  void signalReply(const Key& key) {
    int index = get_index(key);
    if (index < 0) throw std::range_error("[signalReply] key " + std::to_string(key) + " not found!");

    auto& data = _data[index];
    notify(data);
  }

  void waitReply(const Key& key) {
    int index = get_index(key);
    if (index < 0) throw std::range_error("[waitReply] key " + std::to_string(key) + " not found!");

    auto& data = _data[index];
    std::unique_lock<std::mutex> lk(data->mutex);
    while (!data->flag)
      data->cond.wait(lk);
    data->flag.store(false);
  }

  void sendDataWaitReply(const Key& key, Value* value) {
    int index = get_index(key);
    if (index < 0) throw std::range_error("[sendDataWaitReply] key " + std::to_string(key) + " not found!");

    auto& data = _data[index];
    data->val = value;
    std::unique_lock<std::mutex> lk(data->mutex);
#ifdef USE_TBB
    Q.push(index);
#else
    Q.enqueue(index);
#endif
    while (!data->flag)
      data->cond.wait(lk);
    data->flag.store(false);
  }

  inline Value* waitOne() {
    int idx;
#ifdef USE_TBB
    while (true)
      if (Q.try_pop(idx))
        return _data[idx]->val;
#else
    Q.wait_dequeue(idx);
    return _data[idx]->val;
#endif
  }

  inline Value* waitOneUntil(int timeout_sec) {
#ifdef USE_TBB
    int k;
    if (!Q.try_pop(k)) {
      // Sleep would not efficiently return the element.
      std::this_thread::sleep_for(std::chrono::seconds(timeout_sec));
      if (!Q.try_pop(k))
        return nullptr;
    }
    return (_data[k]->val);
#else
    int k;
    if (Q.wait_dequeue_timed(k, timeout_sec))
      return (_data[k]->val);
    else
      return nullptr;
#endif
  }

  // signal reply to all data currently waiting
  void signalReplyAll() {
#ifdef USE_TBB
    std::this_thread::sleep_for(std::chrono::seconds(2));
    int k;
    while (Q.try_pop(k)) {
      notify(_data[k]);
    }
#else
    // some may lie in queues
    while (true) {
      int k;
      if (Q.wait_dequeue_timed(k, 2)) {
        notify(_data[k]);
      } else {
        break;
      }
    }
#endif

    // some may be held in buffers.
    for (auto& td: _data) {
      if (!td->flag.load())
        notify(td);
    }
  }

};

template <typename A, typename B>
using CollectorT = CollectorWithCCQueue<A, B>;


template <typename Key, typename Value>
class BatchCollectorT: public CollectorT<Key, Value> {
  public:
    using BatchValue = std::vector<Value*>;

    explicit BatchCollectorT(const std::vector<Key> &keys):
      CollectorT<Key, Value>{keys} {}

    // non reentrable
    BatchValue waitBatch(int batch_size) {
      while ((int)_batch.size() < batch_size) {
        _batch.emplace_back(this->waitOne());
      }
      BatchValue ret;
      ret.swap(_batch);
      return ret;
    }

  private:
    std::vector<Value*> _batch;
};

}  // namespace elf
