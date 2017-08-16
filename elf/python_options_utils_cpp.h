/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

//File: python_options_utils_cpp.h

#pragma once

#include <iostream>
#include <string>

#include "pybind_helper.h"

struct ContextOptions {
    // How many simulation threads we are running.
    int num_games = 1;

    // The maximum number of threads per game
    int max_num_threads = 0;

    // History length. How long we should keep the history.
    int T = 1;

    // verbose options.
    bool verbose_comm = false;
    bool verbose_collector = false;

    // Whether we wait for each group or we wait jointly.
    bool wait_per_group = false;

    ContextOptions() {}

    void print() const {
      std::cout << "#Game: " << num_games << std::endl;
      std::cout << "#Max_thread: " << max_num_threads << std::endl;
      std::cout << "T: " << T << std::endl;
      if (verbose_comm) std::cout << "Comm Verbose On" << std::endl;
      if (verbose_collector) std::cout << "Comm Collector On" << std::endl;
      std::cout << "Wait per group: " << (wait_per_group ? "True" : "False") << std::endl;
    }

    REGISTER_PYBIND_FIELDS(num_games, max_num_threads, T, verbose_comm, verbose_collector, wait_per_group);
};

inline constexpr int get_query_id(int game_id, int thread_id) {
    return ((thread_id + 1) << 24) | game_id;
}

struct MetaInfo {
    // Id of a game instance.
    int id;

    // Id of a game instance and a particular thread of the game instance
    int thread_id;

    // Id used to query neural network, computed from id and thread_id.
    int query_id;

    MetaInfo(int id) : id(id), thread_id(-1) {
        query_id = get_query_id(id, thread_id);
    }

    MetaInfo(const MetaInfo &parent, int new_thread_id) {
        id = parent.id;
        thread_id = new_thread_id;
        query_id = get_query_id(id, thread_id);
    }

    void ChangeThreadID(int new_thread_id) {
        thread_id = new_thread_id;
        query_id = get_query_id(id, thread_id);
    }

    REGISTER_PYBIND_FIELDS(id, thread_id, query_id);
};

