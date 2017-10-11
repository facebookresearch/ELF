#pragma once

#include <string>
#include <sstream>

#include "pybind_helper.h"

namespace mcts {

using namespace std;

struct TSOptions {
    int max_num_moves = 0;
    int num_threads = 16;
    int num_rollout_per_thread = 100;
    bool verbose = false;
    bool persistent_tree = false;
    // [TODO] Not a good design.
    // string pick_method = "strongest_prior";
    string pick_method = "most_visited";
    bool use_prior = false;

    // [TODO] Some hack here.
    float baseline = 3.0;
    float baseline_sigma = 0.3;

    // Pre-added pseudo playout.
    int pseudo_games = 0;

    string info() const {
      stringstream ss;
      ss << "Maximal #moves (0 = no constraint): " << max_num_moves << endl;
      ss << "#Threads: " << num_threads << endl;
      ss << "#Rollout per thread: " << num_rollout_per_thread << endl;
      ss << "Verbose: " << (verbose ? "True" : "False") << endl;
      ss << "Use prior: " << (use_prior ? "True" : "False") << endl;
      ss << "Persistent tree: " << (persistent_tree ? "True" : "False") << endl;
      ss << "#Pseudo game: " << pseudo_games << endl;
      ss << "Pick method: " << pick_method << endl;
      ss << "Baseline: " << baseline << ", baseline_sigma: " << baseline_sigma << endl;
      return ss.str();
    }

    REGISTER_PYBIND_FIELDS(max_num_moves, num_threads, num_rollout_per_thread, verbose, persistent_tree, pick_method, use_prior, baseline, baseline_sigma, pseudo_games);
};

} // namespace mcts
