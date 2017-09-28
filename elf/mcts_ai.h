#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <atomic>
#include <type_traits>
#include "tree_search.h"
#include "ai.h"

namespace elf {

using namespace std;

template <typename S, typename A>
class MCTSAI_Internal_T : public AI_T<S, A> {
public:
    using MCTSAI_Internal = MCTSAI_Internal_T<S, A>;

    MCTSAI_Internal_T(const mcts::TSOptions& options, bool persistent_tree = false) 
        : _options(options), _persistent_tree(persistent_tree) { 
        _ts.reset(new mcts::TreeSearchT<S, A>(_options));
    }

    const mcts::TSOptions &options() const { return _options; }

protected:
    mcts::TSOptions _options;
    unique_ptr<mcts::TreeSearchT<S, A>> _ts;
    bool _persistent_tree;

    MEMBER_FUNC_CHECK(last_opponent_moves)
    template <typename S_ = S, typename std::enable_if<has_func_last_opponent_moves<S_>::value>::type *U = nullptr> 
    void pass_opponent_moves() {
        if (_persistent_tree) {
            for (const A &prev_move : this->s().last_opponent_moves()) {
                _ts->TreeAdvance(prev_move);
            }
        }
    }
    template <typename S_ = S, typename std::enable_if<!has_func_last_opponent_moves<S_>::value>::type *U = nullptr> 
    void pass_opponent_moves() { }

    bool on_act(Tick, A *a, const std::atomic_bool *) override {
        pass_opponent_moves();
        pair<A, float> res = _ts->Run(this->s());
        *a = res.first; 
        cout << "Action: " << *a << " Score:" << res.second << endl << flush;
        _ts->TreeAdvance(*a);
        return true;
    }
};

}  // namespace elf
