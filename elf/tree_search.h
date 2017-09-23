#pragma once
#include <vector>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "primitive.h"

namespace mcts {

using namespace std;

using NodeId = int;
const NodeId NodeIdInvalid = -1;

template<typename S, typename A>
using ForwardFuncT = function<bool (const S &s, const A &a, S *next)>; 

template<typename S>
using VisitFuncT = function<bool (S *s)>;

template<typename S>
using EvalFuncT = function<float (const S &s)>;

template <typename K, typename V>
const V &map_get(const unordered_map<K, V> &m, const K& k, const V &def) {
    auto it = m.find(k);
    if (it == m.end()) return def;
    else return it->second;
}

template <typename K, typename V>
pair<unordered_map<K, V>::const_iterator, bool> map_get(const unordered_map<K, V> &m, const K& k) {
    auto it = m.find(k);
    if (it == m.end()) {
        return make_pair(m.end(), false);
    } else {
        return make_pair(it, true);
    }
}

template <typename K, typename V>
pair<unordered_map<K, V>::iterator, bool> map_get(unordered_map<K, V> &m, const K& k) {
    auto it = m.find(k);
    if (it == m.end()) {
        return make_pair(m.end(), false);
    } else {
        return make_pair(it, true);
    }
}

template <typename K, typename V>
pair<unordered_map<K, V>::iterator, bool> sync_add_entry(unordered_map<K, V> &m, mutex &mut, const K& k, function <V ()> gen) { 
    auto res = map_get(m, k);
    if (res.second) return res;

    // Otherwise allocate
    lock_guard<mutex> lock(mut);

    // We need to do that again to enforce that one node does not get allocated twice. 
    auto res = map_get(m, k);
    if (res.second) return res;

    // Save it back. This visit doesn't count.
    return m.insert(make_pair(k, gen()));
}

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

template <typename S, typename A>
class NodeAllocT;

struct EdgeInfo {
    // From state.
    const float prior;

    // Accumulated reward and #trial.
    float acc_reward;
    int n;

    EdgeInfo(float p) : prior(p), acc_reward(0), n(0) { }
};

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
        auto res = map_get(sa_val_, a);
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
        auto res = sync_add_entry(sa_next_, lock_edge_, a, [&]() { return alloc.Alloc(init); }); 
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
                A a = UCT(node->sa_vals()).first;

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
    TSOptionsT options_;
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

