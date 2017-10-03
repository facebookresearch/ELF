/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include <type_traits>
#include "tree_search_base.h"

namespace mcts {

using namespace std;

// Algorithms.
template <typename Map>
pair<typename Map::key_type, float> UCT(const Map& vals, float count, bool use_prior = true) {
    // Simple PUCT algorithm.
    using A = typename Map::key_type;
    static_assert(is_same<typename Map::mapped_type, EdgeInfo>::value, "key type must be EdgeInfo");

    A best_a = A();
    float max_score = -1.0;
    const float c_puct = 5.0;
    float sqrt_count = sqrt(count);

    for (const auto& action_pair : vals) {
        const A& a = action_pair.first;
        const EdgeInfo &info = action_pair.second;

        float score = (info.acc_reward + 0.5) / (info.n + 1);
        if (use_prior) score += c_puct * info.prior / (1 + info.n) * sqrt_count;

        if (score > max_score) {
            max_score = score;
            best_a = a;
        }
    }
    return make_pair(best_a, max_score);
};

template <typename Map>
pair<typename Map::key_type, float> MostVisited(const Map& vals) {
    using A = typename Map::key_type;
    static_assert(is_same<typename Map::mapped_type, EdgeInfo>::value, "key type must be EdgeInfo");

    A best_a = A();
    float max_score = -1.0;
    for (const pair<A, EdgeInfo> & action_pair : vals) {
        const A& a = action_pair.first;
        const EdgeInfo &info = action_pair.second;

        if (info.n > max_score) {
            max_score = info.n;
            best_a = a;
        }
    }
    return make_pair(best_a, max_score);
};

template <typename Map>
pair<typename Map::key_type, float> StrongestPrior(const Map& vals) {
    using A = typename Map::key_type;
    static_assert(is_same<typename Map::mapped_type, EdgeInfo>::value, "key type must be EdgeInfo");

    A best_a = A();
    float max_score = -1.0;
    for (const pair<A, EdgeInfo> & action_pair : vals) {
        const A& a = action_pair.first;
        const EdgeInfo &info = action_pair.second;

        if (info.prior > max_score) {
            max_score = info.prior;
            best_a = a;
        }
    }
    return make_pair(best_a, max_score);
};

} // namespace mcts
