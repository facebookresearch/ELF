/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once
#include <vector>

// head is next thing to pop.
// tail is next location to push.
template <typename T>
class CircularQueue {
private:
    // Game history. Use circular queue.
    std::vector<T> _q;

    int _head, _tail, _sz;

    inline void _inc(int &v) const {
        v ++;
        if (v >= (int)_q.size()) v = 0;
    }
    inline void _proj(int &v) const {
        while (v >= (int)_q.size()) v -= (int)_q.size();
        while (v < 0) v += (int)_q.size();
    }

public:
    CircularQueue(int hist_len) : _q(hist_len) { clear(); }

    // Return item to be popped.
    const T &ItemPop() const { return _q[_head]; }
    T &ItemPop() { return _q[_head]; }

    // Return item to be pushed.
    const T &ItemPush() const { return _q[_tail]; }
    T &ItemPush() { return _q[_tail]; }

    bool Push() {
        if (full()) return false;
        _inc(_tail);
        _sz ++;
        return true;
    }

    bool Pop() {
        if (empty()) return false;
        _inc(_head);
        _sz --;
        return true;
    }

    T &GetRoom() {
        if (full()) Pop();
        T &v = ItemPush();
        Push();
        return v;
    }

    std::vector<T> &v() { return _q; }

    const T &get_from_push(int i) const {
        // i == 0 is the object we just pushed.
        // Note that it does not check out of bound!
        int idx = _tail - 1 - i;
        _proj(idx);
        return _q[idx];
    }

    T &get_from_push(int i) {
        // i == 0 is the object we just pushed.
        // Note that it does not check out of bound!
        int idx = _tail - 1 - i;
        _proj(idx);
        return _q[idx];
    }

    int size() const { return _sz; }
    int maxlen() const { return (int)_q.size(); }
    bool empty() const { return _sz == 0; }
    bool full() const { return _sz == (int)_q.size(); }
    void clear() {
        _head = 0; _tail = 0; _sz = 0;
        _q.resize(_q.size(), T());
    }
};


