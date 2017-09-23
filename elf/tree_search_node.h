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

template <typename S, typename A>
class NodeAllocT;

// Tree node.
template <typename S, typename A>
class NodeT {
public:
    NodeT(VisitFuncT<S> f = nullptr) : visited_(false), count_(0) { f(&s_); }
    NodeT(const NodeT<S, A>&) = delete;
    NodeT<S, A> &operator=(const NodeT<S, A>&) = delete;

    const S &state() const { return s_; } 
    const unordered_map<A, EdgeInfo> &sa_vals() const { return sa_val_; }

    bool Visit(VisitFuncT<S> f) {
        if (visited_) return true;

        // Otherwise visit.  
        lock_guard<mutex> lock(lock_node_);
        if (! f(&s_)) return false;

        // Then we need to allocate sa_val_
        // Assume S.pi() exists and is iteratable.
        for (const pair<A, float> & action_pair : s_.pi()) {
            sa_val_.insert(make_pair(action_pair.first, EdgeInfo(action_pair.second)));
        }

        // Once sa_val_ is allocated, its structure won't change. 
        if (sa_val_.empty()) return false;

        visited_ = true;
        return true;
    }

    bool AccumulateStats(const Action &a, float reward) {
        auto res = elf_utils::map_get(sa_val_, a);
        // Not found, skip
        if (! res.second) return false;

        // Inc #visited 
        count_ ++;

        EdgeInfo &info = res.first.second;
        // Async modification (we probably need to add a locker in the future, or not for speed).
        info.accu_reward += reward;
        info.n ++;
        return true;
    }

    // Expand a new node.
    pair<NodeId, bool> Expand(const A &a, ForwardFuncT<S, A> f, NodeAllocT<S, A> &alloc) {
        auto init = [&](S *next_s) { return f(s_, a, next_s); };
        auto res = elf_utils::sync_add_entry(sa_next_, lock_edge_, a, [&]() { return alloc.Alloc(init); }); 
        // <NodeId, whether it is new>
        return make_pair(*res.first.second, res.second);
    }
    
private:
    // For state. 
    mutex lock_node_;
    S s_;
    atomic_bool visited_;
    unordered_map<A, EdgeInfo> sa_val_;

    // For state and action.
    mutex lock_edge_;
    unordered_map<A, NodeId> sa_next_;

    int count_;
};

template <typename S, typename A>
class NodeAllocT {
public:
    using Node = NodeT<S, A>;

    NodeAllocT(const NodeAllocT<S, A>&) = delete;
    NodeAllocT<S, A> &operator=(const NodeAllocT<S, A>&) = delete;

    void Clear() {
        allocated_.clear();
        root_ready_.reset();
        root_.reset(nullptr);
    }

    void SetRootState(const S& s) {
        root_.reset(new Node([&](S *_s) { *_s = s; }));
        root_ready_.notify(); 
    }

    NodeId Alloc(VisitFuncT<S> f) {
        allocated_.emplace_back(new Node(f));
        return allocated_.size() - 1;
    }

    Node *root() { 
        // Block if root is not set yet.
        root_ready_.wait(1);
        return root_.get();
    }

    const Node *operator[](NodeId i) const { return allocated_[i].get(); }
    Node *operator[](NodeId i) { return allocated_[i].get(); }

private:
    // TODO: We might just allocate one chunk at a time. 
    vector<unique_ptr<Node>> allocated_;
    unique_ptr<Node> root_;
    SemaCollector root_ready_; 
};

}  // namespace std
