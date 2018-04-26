/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include <string>
#include <sstream>

#include "pybind_helper.h"
#include "utils.h"

namespace mcts {

using namespace std;

struct TSOptions {
    int max_num_moves = 0;
    int num_threads = 16;
    int num_rollout_per_thread = 100;
    bool verbose = false;
    bool verbose_time = false;

    string save_tree_filename;

    bool persistent_tree = false;
    // [TODO] Not a good design.
    // string pick_method = "strongest_prior";
    string pick_method = "most_visited";
    bool use_prior = false;

    // Pre-added pseudo playout.
    int pseudo_games = 0;

    string info() const {
      stringstream ss;
      ss << "Maximal #moves (0 = no constraint): " << max_num_moves << endl;
      ss << "#Threads: " << num_threads << endl;
      ss << "#Rollout per thread: " << num_rollout_per_thread << endl;
      ss << "Verbose: " << elf_utils::print_bool(verbose) << ", Verbose_time: " << elf_utils::print_bool(verbose_time) << endl;
      if (! save_tree_filename.empty())
        ss << "Save tree filename: " << save_tree_filename << endl;
      ss << "Use prior: " << elf_utils::print_bool(use_prior) << endl;
      ss << "Persistent tree: " << elf_utils::print_bool(persistent_tree) << endl;
      ss << "#Pseudo game: " << pseudo_games << endl;
      ss << "Pick method: " << pick_method << endl;
      return ss.str();
    }

    REGISTER_PYBIND_FIELDS(max_num_moves, num_threads, num_rollout_per_thread, verbose, persistent_tree, pick_method, use_prior, pseudo_games, verbose_time, save_tree_filename);
};

} // namespace mcts
