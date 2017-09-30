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

template<typename AI>
class AIFactory {
public:
    using RegFunc = std::function<AI *(const std::string &spec)>;

    // Factory method given specification.
    static AI *CreateAI(const std::string &name, const std::string& spec) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _factories.find(name);
        if (it == _factories.end()) return nullptr;
        return it->second(spec);
    }
    static void RegisterAI(const std::string &name, RegFunc reg_func) {
        std::lock_guard<std::mutex> lock(_mutex);
        _factories.insert(std::make_pair(name, reg_func));
    }
private:
    static std::map<std::string, RegFunc> _factories;
    static std::mutex _mutex;
};

template<typename AI>
std::map<std::string, std::function<AI *(const std::string &spec)>> AIFactory<AI>::_factories;

template<typename AI>
std::mutex AIFactory<AI>::_mutex;


