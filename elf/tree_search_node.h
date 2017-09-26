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
#include "tree_search_base.h"

namespace mcts {

using namespace std;

template <typename A>
class NodeAllocT;

// Tree node.
template <typename A>
class NodeT {
public:
    NodeT() : visited_(false), count_(0) { }
    NodeT(const NodeT<A>&) = delete;
    NodeT<A> &operator=(const NodeT<A>&) = delete;

    const unordered_map<A, EdgeInfo> &sa() const { return sa_; }
    int count() const { return count_; }

    bool visited() const { return visited_; }

    bool Expand(const vector<pair<A, float>> &pi, float V, NodeAllocT<A> &alloc) {
        if (visited_) return true;

        // Otherwise visit.  
        lock_guard<mutex> lock(lock_node_);
        if (visited_) return true;
        
        // Then we need to allocate sa_val_
        for (const pair<A, float> & action_pair : pi) {
            EdgeInfo edge(action_pair.second);
            edge.next = alloc.Alloc();
            sa_.insert(make_pair(action_pair.first, std::move(edge)));
        }

        // value
        V_ = V;

        // Once sa_ is allocated, its structure won't change. 
        visited_ = true;
        return true;
    }

    bool AccumulateStats(const A &a, float reward) {
        auto res = elf_utils::map_get(sa_, a);
        // Not found, skip
        if (! res.second) return false;

        // Inc #visited 
        count_ ++;

        EdgeInfo &info = res.first->second;
        // Async modification (we probably need to add a locker in the future, or not for speed).
        info.acc_reward += reward;
        info.n ++;
        return true;
    }

    NodeId Descent(const A &a) const {
        auto res = elf_utils::map_get(sa_, a);
        if (! res.second) return NodeIdInvalid;
        return res.first->second.next;
    }
    
private:
    // For state. 
    mutex lock_node_;
    atomic_bool visited_;
    unordered_map<A, EdgeInfo> sa_;

    int count_;
    float V_;
};

template <typename A>
class NodeAllocT {
public:
    using Node = NodeT<A>;

    NodeAllocT() : allocated_node_count_(0) { 
        root_id_ = Alloc(); 
    }

    NodeAllocT(const NodeAllocT<A>&) = delete;
    NodeAllocT<A> &operator=(const NodeAllocT<A>&) = delete;

    void Clear() {
        allocated_.clear();
    }

    void TreeAdvance(const A& a) {
        NodeId next_root = NodeIdInvalid;
        Node *r = root();

        for (const auto &p : r->sa()) {
            if (p.first == a) next_root = p.second.next;
            else RecursiveFree(p.second.next);
        }
        // Free root.
        Free(root_id_);
        if (next_root == NodeIdInvalid) {
            root_id_ = Alloc();
        } else {
            root_id_ = next_root;
        }
    }

    Node *root() { return (*this)[root_id_]; }

    // Low level functions. 
    NodeId Alloc() {
        lock_guard<mutex> lock(alloc_mutex_);
        allocated_[allocated_node_count_].reset(new Node());
        return allocated_node_count_ ++;
    }

    void Free(NodeId id) {
        allocated_.erase(id);
    }

    void RecursiveFree(NodeId id) {
        Node *root = (*this)[id];
        for (const auto &p : root->sa()) {
            RecursiveFree(p.second.next);
        }
        Free(id);
    }

    const Node *operator[](NodeId i) const { 
        lock_guard<mutex> lock(alloc_mutex_);
        auto it = allocated_.find(i);
        if (it == allocated_.end()) return nullptr;
        else return it->second.get();
    }
    Node *operator[](NodeId i) { 
        lock_guard<mutex> lock(alloc_mutex_);
        auto it = allocated_.find(i);
        if (it == allocated_.end()) return nullptr;
        else return it->second.get();
    }

private:
    // TODO: We might just allocate one chunk at a time. 
    unordered_map<NodeId, unique_ptr<Node>> allocated_;
    NodeId allocated_node_count_;
    mutex alloc_mutex_;
    NodeId root_id_;
};


}  // namespace mcts
