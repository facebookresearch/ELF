#include <unordered_map>

#include "tree_search_base.h"

namespace ts_alg {

using namespace std;

// Algorithms. 
template <typename A>
pair<A, float> UCT(const unordered_map<A, EdgeInfo>& vals, bool use_prob = true) {
    // Simple PUCT algorithm.
    A best_a;
    float max_score = -1.0;
    const float c_puct = 5.0;
    for (const pair<A, EdgeInfo> & action_pair : vals) {
        const A& a = action_pair.first;
        const EdgeInfo &info = action_pair.second;

        float prior = use_prob ? info.prior : 1.0;

        float Q = (info.acc_reward + 0.5) / (info.n + 1);
        float p = prior / (1 + info.n) * sqrt(count_);
        float score = Q + c_puct * p;

        if (score > max_score) {
            max_score = score;
            best_a = a;
        }
    }
    return make_pair(best_a, max_score);
};

template <typename A>
pair<A, float> MostVisited(const unordered_map<A, EdgeInfo>& vals) {
    A best_a;
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

} // namespace ts_alg
