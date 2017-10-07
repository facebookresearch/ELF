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
    const float prior;
    NodeId next;

    // Accumulated reward and #trial.
    float acc_reward;
    int n;

    EdgeInfo(float p) : prior(p), next(NodeIdInvalid), acc_reward(0), n(0) { }
};

} // namespace mcts
