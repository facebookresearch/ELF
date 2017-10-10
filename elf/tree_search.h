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
#include <iostream>
#include <functional>
#include <mutex>
#include <string>
#include <fstream>
#include <unordered_map>

#include "member_check.h"
#include "utils.h"
#include "primitive.h"
#include "tree_search_node.h"
#include "tree_search_alg.h"
#include "ctpl_stl.h"

#include "tree_search_options.h"

/*
 * Use the following function of S
 * Copy Constructor: duplicate the state.
 * s.set_thread(int i). Set the thread idx.
 * bool s.forward(const A& a). Forward function that changes the current state to the next state. Return false if the current state is terminal.
 * float s.reward(). Get a reward given the current state.
 * s.evaluate(). Evaluate the current state to get pi/V.
 * s.pi(): return vector<pair<A, float>> for the candidate actions and its prob.
 * s.value(): return a float for the value of current state.
 *
 */

namespace mcts {

using namespace std;

template <typename S, typename A>
class TSOneThreadT {
public:
    using Node = NodeT<S, A>;
    using NodeAlloc = NodeAllocT<S, A>;

    TSOneThreadT(int thread_id, const TSOptions& options) : thread_id_(thread_id), options_(options) {
        if (options_.verbose) {
            output_.reset(new ofstream("tree_search_" + std::to_string(thread_id) + ".txt"));
        }
    }

    void WaitReady() {
        state_ready_.wait(1);
        state_ready_.reset();
    }

    void NotifyReady() {
        state_ready_.notify();
    }

    template <typename Actor>
    bool Run(int run_id, Actor &actor, int num_rollout, NodeAlloc &alloc) {
        (void)run_id;
        Node *root = alloc.root();
        if (root == nullptr || root->s_ptr() == nullptr) {
            cout << "[" << thread_id_ << "] root node is nullptr!" << endl;
            return false;
        }

#define PRINT_MAIN(args) if (output_ != nullptr) { *output_ << "[run=" << run_id << "] " << args << endl << flush; }

#define PRINT_TS(args) if (output_ != nullptr) { *output_ << "[run=" << run_id << "][iter=" << iter << "/" << num_rollout << "]" << args << endl << flush; }

        PRINT_MAIN("Start. actor thread_id: " << actor.info());

        for (int iter = 0; iter < num_rollout; ++iter) {
            // Start from the root and run one path
            vector<pair<Node *, A>> traj;
            Node *node = root;

            bool is_terminal = false;
            int depth = 0;

            while (node != nullptr && node->visited()) {
                A a = UCT(node->sa(), node->count(), options_.use_prior).first;
                PRINT_TS("[depth=" << depth << "] Action: " << a);

                // Save trajectory.
                traj.push_back(make_pair(node, a));
                NodeId next = node->Descent(a);
                PRINT_TS("[depth=" << depth << "] Descent node id: " << next);

                assert(node->s_ptr());

                Node *next_node = alloc[next];

                PRINT_TS("[depth=" << depth << "] Before forward. ");
                is_terminal = next_node == nullptr || ! _forward(node, a, actor, next_node);
                PRINT_TS("[depth=" << depth << "] After forward. ");
                node = next_node;
                PRINT_TS("[depth=" << depth << "] Next node address: " << hex << node << dec);
                depth ++;
            }

            if (! is_terminal) {
                PRINT_TS("Before evaluation. Node: " << hex << node << dec);
                NodeResponseT<A> &resp = actor.evaluate(*node->s_ptr());

                PRINT_TS("After evaluation. Node: " << hex << node << dec);
                node->Expand(resp, alloc);

                PRINT_TS("Expand complete");
            }

            // Now the node points to a recently created node.
            // Evaluate it and backpropagate.
            if (node == nullptr) continue;
            float reward = get_reward(actor, node);

            PRINT_TS("Reward: " << reward << " Start backprop");

            // Add reward back.
            for (const auto &p : traj) {
                p.first->AccumulateStats(p.second, reward);
            }

            PRINT_TS("Done backprop");
        }

        PRINT_MAIN("Done");
        return true;

#undef PRINT_MAIN
#undef PRINT_TS
    }

private:
    int thread_id_;
    const TSOptions &options_;

    SemaCollector state_ready_;
    std::unique_ptr<ostream> output_;

    static float sigmoid(float x) {
        return 1.0 / (1 + exp(-x));
    }

    MEMBER_FUNC_CHECK(reward)
    template <typename Actor, typename S_ = S, typename std::enable_if<has_func_reward<Actor>::value>::type *U = nullptr>
    float get_reward(const Actor &actor, const Node *node) {
        return actor.reward(*node->s_ptr());
    }

    template <typename Actor, typename S_ = S, typename std::enable_if<! has_func_reward<Actor>::value>::type *U = nullptr>
    float get_reward(const Actor &actor, const Node *node) {
        (void)actor;
        return sigmoid((node->value() - options_.baseline) / options_.baseline_sigma);
    }

    template <typename Actor>
    bool _forward(const Node *node, const A &a, Actor &actor, Node *next_node) {
      auto func = [&]() -> S* {
          S *s = new S(*node->s_ptr());
          if (! actor.forward(*s, a)) {
              delete s;
              return nullptr;
          }
          return s;
      };

      return next_node->SetStateIfNull(func);
    }
};

// Mcts algorithm
template <typename S, typename A, typename Actor>
class TreeSearchT {
public:
    using Node = NodeT<S, A>;
    using TSOneThread = TSOneThreadT<S, A>;
    using NodeAlloc = NodeAllocT<S, A>;

    struct RunInfo {
        int num_rollout = 0;
    };

    TreeSearchT(const TSOptions &options, std::function<Actor (int)> actor_gen)
        : pool_(options.num_threads), options_(options) {

        for (int i = 0; i < options.num_threads; ++i) {
            threads_.emplace_back(new TSOneThread(i, options_));
            actors_.emplace_back(actor_gen(i));
        }

        // cout << "#Thread: " << options.num_threads << endl;
        for (int i = 0; i < options.num_threads; ++i) {
            TSOneThread *th = this->threads_[i].get();
            pool_.push([i, this, th](int) {
                int counter = 0;
                while (! this->done_.get()) {
                    th->WaitReady();
                    // cout << "Wake up, counter = " << counter << endl;
                    th->Run(counter, this->actors_[i], this->run_info_.num_rollout, this->alloc_);
                    // if (! ret) cout << "Thread " << i << " got corrupted data" << endl;
                    this->tree_ready_.notify();
                    counter ++;
                }
                this->done_.notify();
            });
        }
    }

    Actor &actor(int i) { return actors_[i]; }

    pair<A, float> Run(const S& root_state) {
        Node *root = alloc_.root();
        if (root == nullptr) {
            cout << "TreeSearch::root cannot be null!" << endl;
            throw std::range_error("TreeSearch::root cannot be null!");
        }
        root->SetStateIfNull([&]() { return new S(root_state); });

        run_info_.num_rollout = options_.num_rollout_per_thread;
        notify_state_ready();

        // Wait until all tree searches are done.
        tree_ready_.wait(pool_.size());
        tree_ready_.reset();

        root = alloc_.root();
        if (root == nullptr) {
            cout << "TreeSearch::root cannot be null!" << endl;
            throw std::range_error("TreeSearch::root cannot be null!");
        }

        // Pick the best solution.
        if (options_.pick_method == "strongest_prior") return StrongestPrior(root->sa());
        else return MostVisited(root->sa());
    }

    void TreeAdvance(const A &a) {
        alloc_.TreeAdvance(a);
    }

    void Clear() {
        alloc_.Clear();
    }

    void Stop() {
        done_.set();

        run_info_.num_rollout = 0;
        // cout << "About to send notify in Stop " << endl;
        notify_state_ready();

        tree_ready_.wait(pool_.size());

        done_.wait(pool_.size());
    }

    ~TreeSearchT() {
        if (! done_.get()) Stop();
    }

private:
    // Multiple threads.
    ctpl::thread_pool pool_;
    vector<unique_ptr<TSOneThread>> threads_;
    vector<Actor> actors_;

    NodeAlloc alloc_;

    RunInfo run_info_;

    TSOptions options_;
    Notif done_;
    SemaCollector tree_ready_;

    void notify_state_ready() {
        for (size_t i = 0; i < threads_.size(); ++i) {
            threads_[i]->NotifyReady();
        }
    }
};

} // namespace mcts

