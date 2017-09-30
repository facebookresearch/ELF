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
#include <unordered_map>

#include "utils.h"
#include "primitive.h"
#include "tree_search_node.h"
#include "tree_search_alg.h"
#include "ctpl_stl.h"

namespace mcts {

struct TSOptions {
    int max_num_moves;
    int num_threads = 16;
    int num_rollout_per_thread = 100;
};

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

template <typename A>
class TSOneThreadT {
public:
    using Node = NodeT<A>;
    using NodeAlloc = NodeAllocT<A>;

    TSOneThreadT(int thread_id) : thread_id_(thread_id) { }

    void WaitReady() { 
        state_ready_.wait(1);
        state_ready_.reset();
    } 

    void NotifyReady() {
        state_ready_.notify();
    }
    
    template <typename S>
    bool Run(int run_id, const S *s, int num_rollout, NodeAlloc &alloc) {
        (void)run_id;
        if (s == nullptr) return false;
        Node *root = alloc.root();
        if (root == nullptr) return false;

        for (int iter = 0; iter < num_rollout; ++iter) {
            // Start from the root and run one path
            vector<pair<Node *, A>> traj;
            Node *node = root;
            S curr_s(*s);
            curr_s.set_thread(thread_id_);

            bool is_terminal = false;
            int depth = 0;

            while (node->visited()) {
                A a = UCT(node->sa(), node->count()).first;
                // cout << "[run=" << run_id << "][" << iter << "][depth=" << depth << "] Action " << a << endl;

                // Save trajectory.
                traj.push_back(make_pair(node, a));
                NodeId next = node->Descent(a);
                if (! curr_s.forward(a)) {
                    is_terminal = true;
                    break;
                }
                node = alloc[next];
                depth ++;
            }

            if (! is_terminal) {
                curr_s.evaluate();
                node->Expand(curr_s.pi(), curr_s.value(), alloc);
            }

            // Now the node points to a recently created node.
            // Evaluate it and backpropagate.
            float reward = curr_s.reward();

            // Add reward back. 
            for (const auto &p : traj) {
                p.first->AccumulateStats(p.second, reward);
            }
        }
        return true;
    }

private:
    int thread_id_;
    SemaCollector state_ready_; 
};

// Mcts algorithm
template <typename S, typename A>
class TreeSearchT {
public:
    using Node = NodeT<A>;
    using TSOneThread = TSOneThreadT<A>;

    struct RunInfo {
        const S* s = nullptr;
        int num_rollout = 0;
    };

    TreeSearchT(const TSOptions &options) 
        : pool_(options.num_threads), options_(options) {

        for (int i = 0; i < options.num_threads; ++i) {
            threads_.emplace_back(new TSOneThread(i));
        }

        // cout << "#Thread: " << options.num_threads << endl;
        for (int i = 0; i < options.num_threads; ++i) {
            TSOneThread *th = this->threads_[i].get();
            pool_.push([i, this, th](int) {
                int counter = 0;
                while (! this->done_.get()) {
                    th->WaitReady();
                    // cout << "Wake up, counter = " << counter << endl;
                    th->Run(counter, this->run_info_.s, this->run_info_.num_rollout, this->alloc_);
                    // if (! ret) cout << "Thread " << i << " got corrupted data" << endl;
                    this->tree_ready_.notify();
                    counter ++;
                }
                this->done_.notify();
            });
        }
    }

    pair<A, float> Run(const S& root_state) {
        Node *root = alloc_.root();
        if (root == nullptr) {
            cout << "TreeSearch::root cannot be null!" << endl;
            throw std::range_error("TreeSearch::root cannot be null!");
        }

        run_info_.s = &root_state;
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
        return MostVisited(root->sa());
    }

    void TreeAdvance(const A &a) {
        alloc_.TreeAdvance(a);
    }

    void Stop() {
        done_.set();

        run_info_.s = nullptr;
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

    NodeAllocT<A> alloc_; 

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

