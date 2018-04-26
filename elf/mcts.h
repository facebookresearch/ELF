/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <atomic>
#include <type_traits>
#include "utils.h"
#include "tree_search.h"
#include "member_check.h"
#include "ai.h"

namespace elf {

using namespace std;

template <typename Actor>
class MCTSAI_T : public AI_T<typename Actor::State, typename Actor::Action> {
public:
    using State = typename Actor::State;
    using Action = typename Actor::Action;

    using MCTSAI = MCTSAI_T<Actor>;
    using TreeSearch = mcts::TreeSearchT<State, Action, Actor>;

    MCTSAI_T(const mcts::TSOptions &options, std::function<Actor *(int)> gen)
        : options_(options) {
        ts_.reset(new TreeSearch(options_, gen));
    }

    const mcts::TSOptions &options() const { return options_; }
    TreeSearch *GetEngine() { return ts_.get(); }

    bool Act(const State &s, Action *a, const std::atomic_bool *) override {
        if (! options_.persistent_tree) {
            reset_tree();
        } else {
            advance_moves(s);
        }

        if (options_.verbose_time) {
            elf_utils::MyClock clock;
            clock.Restart();

            auto res = ts_->Run(s);
            *a = res.best_a;

            clock.Record("MCTS");
            cout << "[" << this->id() << "] MCTSAI Result: " << res.info() << " Action:" << res.best_a << endl;
            cout << clock.Summary() << endl;
        } else {
            auto res = ts_->Run(s);
            *a = res.best_a;
        }
        return true;
    }

    bool GameEnd() override {
        reset_tree();
        return true;
    }

    /*
    MEMBER_FUNC_CHECK(Restart)
    template <typename Actor_ = Actor, typename std::enable_if<has_func_Restart<Actor_>::value>::type *U = nullptr>
    bool GameEnd() override {
        for (size_t i = 0; i < ts_->size(); ++i) {
            ts_->actor(i).Restart();
        }
        return true;
    }
    */

protected:
    void on_set_id() override {
        for (size_t i = 0; i < ts_->size(); ++i) {
            ts_->actor(i).SetId(this->id());
        }
    }

private:
    mcts::TSOptions options_;
    unique_ptr<TreeSearch> ts_;
    int move_number_;

    void reset_tree() {
        ts_->Clear();
        move_number_ = -1;
    }

    // Note that moves_since should have the following signature.
    //   const vector<Action> &moves_since(int *move_number)
    // It will compare the current move_number to the move number in the state, and return moves since the last move number.
    // If the input move_number is negative, then it will return all moves since the game started.
    // Once it is done, the move_number will be advanced to the most recent move number.
    MEMBER_FUNC_CHECK(moves_since)
    template <typename S_ = State, typename std::enable_if<has_func_moves_since<S_>::value>::type *U = nullptr>
    void advance_moves(const S_ &s) {
        auto recent_moves = s.moves_since(&move_number_);
        for (const Action &prev_move : recent_moves) {
            ts_->TreeAdvance(prev_move);
        }
    }
    template <typename S_ = State, typename std::enable_if<!has_func_moves_since<S_>::value>::type *U = nullptr>
    void advance_moves(const S_ &) {
    }
};

template <typename Actor, typename AIComm, typename ActorParam = void>
class MCTSAIWithCommT : public AI_T<typename Actor::State, typename Actor::Action> {
public:
    using MCTSAI = MCTSAI_T<Actor>;
    using State = typename Actor::State;
    using Action = typename Actor::Action;

    MCTSAIWithCommT(AIComm *ai_comm, const mcts::TSOptions &options)
        : ai_comm_(ai_comm) {
        static_assert(std::is_same<ActorParam, void>::value, "The constructor requires ActorParam to be void (or omitted)");
        // Construct a few DirectPredictAIs.
        spawn_aicomms(options.num_threads);

        // cout << "#ai = " << ai_dup.size() << endl;
        auto actor_gen = [&](int i) { return new Actor(ai_comms_[i].get()); };
        // cout << "Done with MCTSAI_T::InitAIComm" << endl;
        mcts_ai_.reset(new MCTSAI(options, actor_gen));
    }

    MCTSAIWithCommT(AIComm *ai_comm, const mcts::TSOptions &options, const ActorParam *params)
        : ai_comm_(ai_comm) {
        static_assert(! std::is_same<ActorParam, void>::value, "The constructor requires ActorParam to be set (not void)");
        // Construct a few DirectPredictAIs.
        spawn_aicomms(options.num_threads);

        // cout << "#ai = " << ai_dup.size() << endl;
        auto actor_gen = [&](int i) { return new Actor(ai_comms_[i].get(), *params); };
        // cout << "Done with MCTSAI_T::InitAIComm" << endl;
        mcts_ai_.reset(new MCTSAI(options, actor_gen));
    }

    bool Act(const State &s, Action *a, const std::atomic_bool *done) override {
        return mcts_ai_->Act(s, a, done);
    }

    MCTSAI *get() { return mcts_ai_.get(); }

    bool GameEnd() override {
        // Restart _ai_comm.
        ai_comm_->Restart();
        for (auto &ai_comm : ai_comms_) {
            ai_comm->Restart();
        }
        return mcts_ai_->GameEnd();
    }

protected:
    void on_set_id() override {
        mcts_ai_->SetId(this->id());
    }

private:
    AIComm *ai_comm_;
    vector<unique_ptr<AIComm>> ai_comms_;
    unique_ptr<MCTSAI> mcts_ai_;

    void spawn_aicomms(int num_threads) {
        // Construct a few DirectPredictAIs.
        ai_comms_.clear();
        for (int i = 0; i < num_threads; ++i) {
            ai_comms_.emplace_back(ai_comm_->Spawn(i));
        }
    }

    // cout << "#ai = " << ai_dup.size() << endl;
    std::function<Actor *(int)> actor_gen() {
        return [&](int i) { return new Actor(ai_comms_[i].get()); };
    }
};

}  // namespace elf
