/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#pragma once

#include "cmd_receiver.h"
#include "game_state.h"
#include "elf/ai.h"
#include <atomic>
#include <chrono>
#include <algorithm>

using namespace std;

template<typename AI>
class AIFactory {
public:
    using RegFunc = function<AI *(const string &spec)>;

    // Factory method given specification.
    static AI *CreateAI(const string &name, const string& spec) {
        lock_guard<mutex> lock(_mutex);
        auto it = _factories.find(name);
        if (it == _factories.end()) return nullptr;
        return it->second(spec);
    }
    static void RegisterAI(const string &name, RegFunc reg_func) {
        lock_guard<mutex> lock(_mutex);
        _factories.insert(make_pair(name, reg_func));
    }
private:
    static map<string, RegFunc> _factories;
    static mutex _mutex;
};

template<typename AI>
map<string, function<AI *(const string &spec)>> AIFactory<AI>::_factories;

template<typename AI>
mutex AIFactory<AI>::_mutex;


