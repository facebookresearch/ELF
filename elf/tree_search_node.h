/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
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

template<typename A>
struct NodeResponseT {
    vector<pair<A, float>> pi;
    float value;
};

template <typename S, typename A>
class NodeAllocT;

template <typename S>
class NodeBaseT {
public:
    enum StateType { NODE_NULL = 0, NODE_INVALID, NODE_SET };

    NodeBaseT() : s_state_(NODE_NULL) { }

    const S *s_ptr() const { return s_.get(); }
    bool SetStateIfNull(function<S *()> func) {
      if (func == nullptr) return false;
      // The node is invalid.
      int curr_state = s_state_.load();
      if (curr_state == NODE_INVALID) return false;
      if (curr_state == NODE_SET) return true;

      lock_guard<mutex> lock(lock_state_);

      curr_state = s_state_.load();
      if (curr_state == NODE_INVALID) return false;
      if (curr_state == NODE_SET) return true;

      s_.reset(func());
      if (s_ == nullptr) {
        s_state_ = NODE_INVALID;
        return false;
      } else {
        s_state_ = NODE_SET;
        return true;
      }
    }

protected:
    mutex lock_state_;
    unique_ptr<S> s_;
    atomic<int> s_state_;
};

// Tree node.
template <typename S, typename A>
class NodeT : public NodeBaseT<S> {
public:
    using Node = NodeT<S, A>;
    using NodeAlloc = NodeAllocT<S, A>;

    enum VisitType { NODE_NOT_VISITED = 0, NODE_JUST_VISITED, NODE_ALREADY_VISITED };

    NodeT(Node *parent) : parent_(parent), visited_(false), count_(0) { }
    NodeT(const Node&) = delete;
    Node &operator=(const Node&) = delete;

    const unordered_map<A, EdgeInfo> &sa() const { return sa_; }
    int count() const { return count_; }
    float value() const { return V_; }

    template <typename ExpandFunc, typename InitFunc>
    VisitType ExpandIfNecessary(ExpandFunc func, InitFunc init, NodeAlloc &alloc) {
        if (visited_) return NODE_ALREADY_VISITED;

        // Otherwise visit.
        lock_guard<mutex> lock(lock_node_);
        if (visited_) return NODE_ALREADY_VISITED;

        auto resp = func(this);

        // Then we need to allocate sa_val_
        for (const pair<A, float> & action_pair : resp.pi) {
            auto res = sa_.insert(make_pair(action_pair.first, EdgeInfo(action_pair.second)));
            res.first->second.next = alloc.Alloc(this);
            init(res.first->second);
            // Compute v here.
            // Node *child = alloc[res.first->second.next];
            // child->V_ = V_ + log(action_pair.second + 1e-6);
        }

        // value
        V_ = resp.value;

        // Once sa_ is allocated, its structure won't change.
        visited_ = true;
        return NODE_JUST_VISITED;
    }

    bool AccumulateStats(const A &a, float reward) {
        auto res = elf_utils::map_get(sa_, a);
        // Not found, skip
        if (! res.second) return false;

        // Inc #visited
        count_ ++;

        EdgeInfo &info = res.first->second;
        // Async modification (we probably need to add a locker in the future, or not for speed).
        lock_guard<mutex> lock(lock_node_);
        info.acc_reward += reward;
        info.n ++;
        return true;
    }

    NodeId Descent(const A &a) const {
        auto res = elf_utils::map_get(sa_, a);
        if (! res.second) return NodeIdInvalid;
        return res.first->second.next;
    }

    string _info(int indent, const NodeAlloc &alloc) const {
        std::stringstream ss;
        std::string indent_str;
        for (int i = 0; i < indent; ++i) indent_str += ' ';

        for (const auto & p : sa_) {
            if (p.second.n > 0) {
                const Node *n = alloc[p.second.next];
                if (n->visited_) {
                    ss << indent_str << "[" << p.first << "] " << p.second.info();
                    ss << ", V: " << n->V_ << endl;
                    ss << n->_info(indent + 2, alloc);
                }
            }
        }
        return ss.str();
    }

    string info(const NodeAlloc &alloc) const {
        return _info(0, alloc);
    }

private:
    // For state.
    Node *parent_;
    mutex lock_node_;
    atomic_bool visited_;
    unordered_map<A, EdgeInfo> sa_;

    atomic<int> count_;
    float V_ = 0.0;
};

template <typename S, typename A>
class NodeAllocT {
public:
    using Node = NodeT<S, A>;
    using NodeAlloc = NodeAllocT<S, A>;

    NodeAllocT() { Clear(); }

    NodeAllocT(const NodeAlloc&) = delete;
    NodeAlloc &operator=(const NodeAlloc&) = delete;

    void Clear() {
        allocated_.clear();
        allocated_node_count_ = 0;
        root_id_ = Alloc();
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
    const Node *root() const { return (*this)[root_id_]; }

    // Low level functions.
    NodeId Alloc(Node *parent = nullptr) {
        lock_guard<mutex> lock(alloc_mutex_);
        allocated_[allocated_node_count_].reset(new Node(parent));
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

    Node *operator[](NodeId i) {
        lock_guard<mutex> lock(alloc_mutex_);
        auto it = allocated_.find(i);
        if (it == allocated_.end()) return nullptr;
        else return it->second.get();
    }

    const Node *operator[](NodeId i) const {
        lock_guard<mutex> lock(alloc_mutex_);
        auto it = allocated_.find(i);
        if (it == allocated_.end()) return nullptr;
        else return it->second.get();
    }

private:
    // TODO: We might just allocate one chunk at a time.
    unordered_map<NodeId, unique_ptr<Node>> allocated_;
    NodeId allocated_node_count_;
    mutable mutex alloc_mutex_;
    NodeId root_id_;
};


}  // namespace mcts
