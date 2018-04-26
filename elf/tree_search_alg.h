/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include <type_traits>
#include "tree_search_base.h"

namespace mcts {

using namespace std;

// Algorithms.
template <typename Map>
pair<typename Map::key_type, float> UCT(const Map& vals, float count, bool use_prior = true, ostream *oo = nullptr) {
    // Simple PUCT algorithm.
    using A = typename Map::key_type;
    static_assert(is_same<typename Map::mapped_type, EdgeInfo>::value, "key type must be EdgeInfo");

    A best_a = A();
    float max_score = std::numeric_limits<float>::lowest();
    const float c_puct = 0.5;
    const float sqrt_count1 = sqrt(count + 1);

    if (oo) *oo << "UCT prior = " << (use_prior ? "True" : "False") << endl;

    for (const auto& action_pair : vals) {
        const A& a = action_pair.first;
        const EdgeInfo &info = action_pair.second;

        float score = (info.acc_reward + 0.5) / (info.n + 1);
        if (use_prior) score += c_puct * info.prior / (1 + info.n) * sqrt_count1;

        if (oo) *oo << "UCT [a=" << a << "] prior: " << info.prior << " score: " <<  score << endl;

        if (score > max_score) {
            max_score = score;
            best_a = a;
        }
    }
    return make_pair(best_a, max_score);
};

template <typename Map>
MCTSResultT<typename Map::key_type> MostVisited(const Map& vals) {
    using A = typename Map::key_type;
    using MCTSResult = MCTSResultT<A>;
    static_assert(is_same<typename Map::mapped_type, EdgeInfo>::value, "key type must be EdgeInfo");

    MCTSResult res;
    for (const pair<A, EdgeInfo> & action_pair : vals) {
        const EdgeInfo &info = action_pair.second;

        res.feed(info.n, action_pair);
    }
    return res;
};

template <typename Map>
MCTSResultT<typename Map::key_type> StrongestPrior(const Map& vals) {
    using A = typename Map::key_type;
    using MCTSResult = MCTSResultT<A>;
    static_assert(is_same<typename Map::mapped_type, EdgeInfo>::value, "key type must be EdgeInfo");

    MCTSResult res;
    for (const pair<A, EdgeInfo> & action_pair : vals) {
        const EdgeInfo &info = action_pair.second;

        res.feed(info.prior, action_pair);
    }
    return res;
};

template <typename Map>
MCTSResultT<typename Map::key_type> UniformRandom(const Map& vals) {
    using A = typename Map::key_type;
    using MCTSResult = MCTSResultT<A>;
    static_assert(is_same<typename Map::mapped_type, EdgeInfo>::value, "key type must be EdgeInfo");

    static std::mt19937 rng(time(NULL));
    static std::mutex mu;

    MCTSResult res;

    int idx = 0;
    {
        lock_guard<mutex> lock(mu);
        idx = rng() % vals.size();
    }
    auto it = vals.begin();
    while (--idx >= 0) ++ it;

    res.feed(it->second.n, *it);
    return res;
};

} // namespace mcts
