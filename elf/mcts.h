#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <atomic>
#include <type_traits>
#include "tree_search.h"
#include "member_check.h"
#include "ai.h"

namespace elf {

using namespace std;

template <typename Actor>
class MCTSAI_T : public AI_T<typename Actor::State, typename Actor::Action> {
public:
    using AI = typename Actor::AI;
    using State = typename Actor::State;
    using Action = typename Actor::Action;

    using MCTSAI = MCTSAI_T<Actor>;
    using TreeSearch = mcts::TreeSearchT<State, Action, Actor>;

    MCTSAI_T(const mcts::TSOptions &options, std::function<Actor (int)> gen) : options_(options) {
        ts_.reset(new TreeSearch(options_, gen));
    }

    const mcts::TSOptions &options() const { return options_; }
    TreeSearch *GetEngine() { return ts_.get(); }

    bool Act(const State &s, Action *a, const std::atomic_bool *) override {
        pass_opponent_moves(s);
        pair<Action, float> res = ts_->Run(s);
        *a = res.first;
        ts_->TreeAdvance(*a);
        return true;
    }

private:
    mcts::TSOptions options_;
    unique_ptr<TreeSearch> ts_;

    MEMBER_FUNC_CHECK(last_opponent_moves)
    template <typename S_ = State, typename std::enable_if<has_func_last_opponent_moves<S_>::value>::type *U = nullptr>
    void pass_opponent_moves(const S_ &s) {
        if (options_.persistent_tree) {
            for (const Action &prev_move : s.last_opponent_moves()) {
                ts_->TreeAdvance(prev_move);
            }
        } else {
            // Clear the tree.
            ts_->Clear();
       }
    }
    template <typename S_ = State, typename std::enable_if<!has_func_last_opponent_moves<S_>::value>::type *U = nullptr>
    void pass_opponent_moves(const S_ &) {
        ts_->Clear();
    }
};

template <typename Actor, typename AIComm>
class MCTSAIWithCommT : public AI_T<typename Actor::State, typename Actor::Action> {
public:
    using MCTSAI = MCTSAI_T<Actor>;
    using AI = typename Actor::AI;
    using State = typename Actor::State;
    using Action = typename Actor::Action;

    MCTSAIWithCommT(AIComm *ai_comm, const mcts::TSOptions &options)
        : ai_comm_(ai_comm) {
        // Construct a few DirectPredictAIs.
        ai_.clear();
        ai_comms_.clear();

        for (int i = 0; i < options.num_threads; ++i) {
            ai_comms_.emplace_back(ai_comm_->Spawn(i));
            ai_.emplace_back(new AI());
            ai_.back()->InitAIComm(ai_comms_.back().get());
        }

        // cout << "#ai = " << ai_dup.size() << endl;
        auto actor_gen = [&](int i) { return Actor(ai_[i].get()); };
        // cout << "Done with MCTSAI_T::InitAIComm" << endl;
        mcts_ai_.reset(new MCTSAI(options, actor_gen));
    }

    bool Act(const State &s, Action *a, const std::atomic_bool *done) override {
        return mcts_ai_->Act(s, a, done);
    }

    MCTSAI *get() { return mcts_ai_.get(); }

    bool GameEnd(const State &s) override {
        // Send final message.
        Action action;
        Act(s, &action, nullptr);

        // Restart _ai_comm.
        ai_comm_->Restart();
        for (size_t i = 0; i < ai_comms_.size(); ++i) {
            ai_comms_[i]->Restart();
        }
        return true;
    }

protected:
    void on_set_id() override {
        for (size_t i = 0; i < ai_.size(); ++i) {
            ai_[i]->SetId(this->id());
        }
    }

private:
    AIComm *ai_comm_;

    vector<unique_ptr<AIComm>> ai_comms_;
    vector<unique_ptr<AI>> ai_;

    unique_ptr<MCTSAI> mcts_ai_;
};

}  // namespace elf
