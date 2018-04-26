/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once
#include <atomic>

namespace elf {

class Signal {
public:
    Signal(const std::atomic_bool &d, const std::atomic_bool &prepare_stop)
      : done_(d), prepare_stop_(prepare_stop) {
    }

    bool IsDone() const { return done_.load(); }
    const std::atomic_bool &done() const { return done_; }
    bool PrepareStop() const { return prepare_stop_.load(); }

private:
    const std::atomic_bool &done_;
    const std::atomic_bool &prepare_stop_;
};

}  // namespace elf
