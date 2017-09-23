#pragma once
#include <vector>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "utils.h"
#include "primitive.h"
#include "tree_search_node.h"
#include "tree_search_alg.h"

namespace mcts {

template <typename S, typename A>
struct TSOptionsT {
    using VisitFunc = VisitFuncT<S>;
    using ForwardFunc = ForwardFuncT<S, A>;
    using EvalFunc = EvalFuncT<S>;

    int num_thread = 16;
    int num_rollout_per_thread = 100;
    int max_num_moves;

    VisitFunc stats_func = nullptr;
    ForwardFunc forward_func = nullptr;
    EvalFunc evaluate = nullptr;
};

template <typename S, typename A>
class TSOneThread {
public:
    using Node = NodeT<S, A>;
    using NodeAlloc = NodeAllocT<S, A>;
    using VisitFunc = VisitFuncT<S>;
    using ForwardFunc = ForwardFuncT<S, A>;
    using EvalFunc = EvalFuncT<S>;
    using TSOptions = TSOptionsT<S, A>;

    TSOneThread(const TSOptions& options) : options_(options) { }
    
    void Run(NodeAlloc &alloc) {
        for (int iter = 0; iter < options_.num_rollout_per_thread; ++iter) {
            // Start from the root and run one path
            Node *node = alloc.root();
            vector<pair<Node *, A>> traj;
            while (true) {
                // Save trajectory.
                if (! node->Visit(options_.stats_func)) break;
                A a = ts_alg::UCT(node->sa_vals()).first;

                traj.push_back(make_pair(node, a));
                auto next = node->Expand(a, options_.forward_func, alloc);

                // Compute moves.
                node = alloc[next.first];
                if (next.second) break;
            }
            // Now the node points to a recently created node.
            // Evaluate it and backpropagate.
            float reward = options_.evaluate(node->state());

            // Add reward back. 
            for (const auto &p : traj) {
                p.first->AccumulateStats(p.second, reward);
            }
        }
    }

private:
    TSOptions options_;
};

// Mcts algorithm
template <typename S, typename A>
class TreeSearch {
public:
    TreeSearch(const TSOptions &options) : pool_(options.num_thread), options_(options) {
        for (int i = 0; i < options.num_thread; ++i) {
            pool_.push([this](int) {
                TSOneThread game(this->options_);
                game.Run(this->alloc_);
                all_threads_done_.notify();
            });
        }
    }

    pair<A, float> Run(const S& root_state) {
        alloc_.Clear();
        alloc_.SetRootState(root_state);

        all_threads_done_.wait(pool_.size());

        // Get the results. 
        Node *node = alloc_.root();
        // Pick the best solution.
        return ts_alg::MostVisited(node->sa_vals());
    }

private:
    // Multiple threads.
    ctpl::thread_pool pool_;
    NodeAllocT<S, A> alloc_; 

    TSOptions options_;
    SemaCollector all_threads_done_;
};

} // namespace mcts

