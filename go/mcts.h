/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include "elf/tree_search.h"
#include "go_state.h"

using namespace std;

// Tree search specialization for Go
//
class MCTSState : public GoState {
public:
    MCTSState(const GoState &state) : GoState(state), value_(0.5) { }
    const vector<pair<Coord, float>> &pi() const { return pi_; }

    bool evaluate(DirectPredictAI *ai) {
        ai->SetState(*this);
        if (! ai->Act(GetPly(), nullptr, nullptr)) return false;
        ai->get_last_pi(&pi_);
        V_ = ai->get_last_value();
        return true;
    }

    vector<Coord> last_opponent_moves() const { 
        Coord m = LastMove2();
        if (m != M_PASS) return vector<Coord>{ LastMove2() }; 
        else return vector<Coord>();
    }

    // Evaluate using random playout.
    // Right now just set it to 0.5
    float reward() const { return value_; }
    float value() const { return value_; }

private:
    vector<pair<Coord, float>> pi_;
    float value_;
};

class MCTSStateMT : public MCTSState {
public:
    MCTSStateMT(const GoState &state, const vector<DirectPredictAI *>& ai) 
        : MCTSState(state), ai_(ai), thread_id_(0) { }

    void set_thread_id(int i) { thread_id_ = i; }

    bool evaluate() {
        return MCTSState::evaluate(ai_[thread_id_]);
    }

private:
    vector<DirectPredictAI *> ai_;
    int thread_id_;
};

class MCTSGoAI : public AI {
public:
    using MCTSAI = elf::MCTSAI<MCTSStateMT, Coord>;

    MCTSGoAI(const mcts::TSOption &options) : mcts_ai_(options), ai_comm_(nullptr) {
    }

    void InitAIComm(AIComm *ai_comm) {
        ai_comm_ = ai_comm;

        // Construct a few DirectPredictAIs.
        const auto &options = mcts_ai_.options();
        ai_.clear(); 
        ai_dup_.clear();
        ai_comms_.clear();
        for (int i = 0; i < options.num_threads; ++i) {
            ai_comms_.emplace_back(ai_comm()->Spawn(i));
            ai_.emplace_back(new DirectPredictAI());
            ai_.back()->InitAIComm(ai_comms_.back().get());
            ai_dup_.emplace_back(ai_.back().get());
        }
    } 

protected:
    void on_set_state() override {
        // Left the state to MCTSStateMT.
        go_state_.reset(new MCTSStateMT(state, ai_dup_));
        mcts_ai_.SetState(*go_state_);
    }

    bool on_act(Tick t, Coord *c, const std::atomic_bool *done) override {
        return mcts_ai_.Act(t, c, done);
    }

private:
    MCTSAI mcts_ai_;
    AIComm *ai_comm_;

    unique_ptr<MCTSStateMT> go_state_;
    vector<unique_ptr<AIComm>> ai_comms_;
    vector<unique_ptr<DirectPredictAI>> ai_;
    vector<DirectPredictAI *> ai_dup_;
};
