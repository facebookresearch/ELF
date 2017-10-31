/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include <functional>
#include <unordered_map>
#include <string>
#include <sstream>

namespace mcts {

using namespace std;

using NodeId = int;
const NodeId NodeIdInvalid = -1;

template<typename S, typename A>
using ForwardFuncT = function<bool (const S &s, const A &a, S *next)>;

template<typename S>
using VisitFuncT = function<bool (S *s)>;

template<typename S>
using EvalFuncT = function<float (const S &s)>;

struct EdgeInfo {
    // From state.
    float prior;
    NodeId next;

    // Accumulated reward and #trial.
    float acc_reward;
    int n;

    EdgeInfo(float p = 0.0) : prior(p), next(NodeIdInvalid), acc_reward(0), n(0) { }

    string info() const {
        std::stringstream ss;
        ss << acc_reward << "/" << n << " (" << acc_reward / n << "), Pr: " << prior << ", next: " << next;
        return ss.str();
    }
};

template <typename A>
struct MCTSResultT {
    A best_a;
    float max_score;
    EdgeInfo edge_info;

    MCTSResultT() : best_a(A()), max_score(std::numeric_limits<float>::lowest()) {
    }

    bool feed(float score, const pair<A, EdgeInfo> &e) {
      if (score > max_score) {
        max_score = score;
        best_a = e.first;
        edge_info = e.second;
        return true;
      }
      return false;
    }

    string info() const {
        std::stringstream ss;
        ss << "BestA: " << best_a << ", MaxScore: " << max_score << ", Info: " << edge_info.info();
        return ss.str();
    }
};

} // namespace mcts
