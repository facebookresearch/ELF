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

    // Accumulated reward and #trial.
    float acc_reward;
    int n;

    EdgeInfo(float p) : prior(p), acc_reward(0), n(0) { }
};

} // namespace mcts
