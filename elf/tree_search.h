/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

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

struct TSOptions {
    int max_num_moves;
};

template <typename S, typename A>
struct TSThreadOptionsT {
    using VisitFunc = VisitFuncT<S>;
    using ForwardFunc = ForwardFuncT<S, A>;
    using EvalFunc = EvalFuncT<S>;

    int num_rollout_per_thread = 100;

    VisitFunc stats_func = nullptr;
    ForwardFunc forward_func = nullptr;
    EvalFunc evaluate = nullptr;
};

template <typename S, typename A>
class TSOneThreadT {
public:
    using Node = NodeT<S, A>;
    using NodeAlloc = NodeAllocT<S, A>;
    using VisitFunc = VisitFuncT<S>;
    using ForwardFunc = ForwardFuncT<S, A>;
    using EvalFunc = EvalFuncT<S>;
    using TSThreadOptions = TSThreadOptionsT<S, A>;

    TSOneThreadT(const TSThreadOptions& options) : options_(options) { }
    
    void Run(NodeAlloc &alloc) {
        for (int iter = 0; iter < options_.num_rollout_per_thread; ++iter) {
            // Start from the root and run one path
            Node *node = alloc.root();
            vector<pair<Node *, A>> traj;
            while (true) {
                // Save trajectory.
                if (! node->Visit(options_.stats_func)) break;
                A a = UCT(node->sa_vals(), node->count()).first;

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
    TSThreadOptions options_;
};

// Mcts algorithm
template <typename S, typename A>
class TreeSearchT {
public:
    using TSThreadOptions = TSThreadOptionsT<S, A>;
    using Node = NodeT<S, A>;

    TreeSearchT(const TSOptions &options, const vector<TSThreadOptions> &thread_options) 
        : pool_(thread_options.size()), options_(options) {
        for (size_t i = 0; i < thread_options.size(); ++i) {
            pool_.push([i, this, thread_options](int) {
                TSOneThreadT<S, A> game(thread_options[i]);
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
        return MostVisited(node->sa_vals());
    }

private:
    // Multiple threads.
    ctpl::thread_pool pool_;
    NodeAllocT<S, A> alloc_; 

    TSOptions options_;
    SemaCollector all_threads_done_;
};

} // namespace mcts

