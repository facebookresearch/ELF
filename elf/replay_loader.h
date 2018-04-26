/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "shared_replay_buffer.h"

namespace elf {

using namespace std;

template <typename K, typename Record>
class ReplayLoaderT {
public:
    using RIterator = typename Record::iterator;
    using RBuffer = SharedReplayBuffer<K, Record>;
    using GenFunc = typename RBuffer::GenFunc;

    static void Init(GenFunc func) {
        _rbuffer.reset(new RBuffer(func));
    }

    void Reload() {
        while (true) {
            K k = get_key();
            const auto &record = _rbuffer->Get(k);
            _it = record.begin();
            if (after_reload(k, _it)) return;
        }
    }

    const RIterator &curr() const { return _it; }
    void increment() { ++ _it; }

private:
    // Shared buffer for OfflineLoader.
    static std::unique_ptr<RBuffer> _rbuffer;

    RIterator _it;

protected:
    // The function return true if the load is valid, otherwise return false.
    virtual bool after_reload(const K& k, RIterator &it) = 0;
    virtual K get_key() = 0;
};

template <typename K, typename Record>
std::unique_ptr<SharedReplayBuffer<K, Record>> ReplayLoaderT<K, Record>::_rbuffer;

}  // namespace elf
