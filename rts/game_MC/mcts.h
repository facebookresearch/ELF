/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include "engine/game_state.h"
#include "trainable_ai.h"
#include "reduced_ai.h"
#include "rule_ai.h"
#include "elf/mcts.h"

/*
class MCTSState : public RTSState {
public:
    using AI = TrainedAI;
    using Action = int;
    using State = RTSState;

    MCTSState &operator=(const State &state) {
        *((State *)this) = state;
        return *this;
    }

    void SetAI(AI *ai) { ai_ = ai; }

    bool evaluate() {
        if (! ai_->ActImmediate(*this)) return false;
        ai_->get_last_pred(&pred_);
        return true;
    }

    const vector<pair<int, float>> &pi() const { return pred_.pi; }
    float value() const { return pred_.value; }
    float reward() const { return pred_.value; }

protected:
    ReducedPred pred_;
    AI *ai_ = nullptr;
};
*/

struct MultiAI {
    void InitAIComm(AIComm *ai_comm) {
        predict.InitAIComm(ai_comm);
        forward.InitAIComm(ai_comm);
        project.InitAIComm(ai_comm);
    }

    void SetId(PlayerId id) {
        predict.SetId(id);
        forward.SetId(id);
        project.SetId(id);
    }

    reduced::PredictAI predict;
    reduced::ForwardAI forward;
    reduced::ProjectAI project;
};

class MCTSReducedActor {
public:
    using Action = int;
    using State = ReducedState;

    using FullState = RTSState;
    using Response = mcts::NodeResponseT<Action>;

    MCTSReducedActor(AIComm *ai_comm)
      : ai_comm_(ai_comm) {
        ai_.InitAIComm(ai_comm);
    }

    bool project(const FullState &state, State *s) {
        auto &ai = ai_.project;
        return ai.Act(state, &s->state, nullptr);
    }

    Response &evaluate(const State &s) {
        auto &ai = ai_.predict;
        ai.Act(s.state, &resp_, nullptr);
        return resp_;
    }

    bool forward(State &s, const Action &a) {
        s.action = a;
        auto &ai = ai_.forward;
        vector<float> next_state;
        if (! ai.Act(s, &next_state, nullptr)) return false;
        s.state = next_state;
        return true;
    }

    string info() const {
        return "";
        // stringstream ss;
        // return ss.str();
    }

    void SetId(PlayerId id) {
        ai_.SetId(id);
    }

protected:
    ReducedPred resp_;
    AIComm *ai_comm_ = nullptr;
    MultiAI ai_;
};

class MCTSRTSReducedAI : public AI {
public:
    MCTSRTSReducedAI(AIComm *ai_comm, const mcts::TSOptions &options) : mcts_ai_(ai_comm, options) {
        // rng_.seed(time(NULL));
    }

    bool Act(const RTSState &s, RTSMCAction *a, const std::atomic_bool *done) override {
        a->Init(id(), name());
        ReducedState reduced_s;
        mcts_ai_.get()->GetEngine()->actor(0).project(s, &reduced_s);
        int reduced_a = int(); // rng_() % 9; //
        if (! mcts_ai_.Act(reduced_s, &reduced_a, done)) return false;
        // cout << "[t=" << s.GetTick() << "] Action: " << reduced_a << endl << flush;
        a->SetState9(reduced_a);
        return true;
    }

    bool GameEnd(const RTSState &s) override {
        // Send final message.
        ReducedState reduced_s;
        mcts_ai_.get()->GetEngine()->actor(0).project(s, &reduced_s);
        return mcts_ai_.GameEnd(reduced_s);
    }

protected:
    void on_set_id() override {
         mcts_ai_.SetId(id());
    }

private:
    elf::MCTSAIWithCommT<MCTSReducedActor, AIComm> mcts_ai_;
    // std::mt19937 rng_;
};

class MCTSActor {
public:
    using Action = int;
    using State = RTSState;
    using Response = mcts::NodeResponseT<Action>;
    using RTSGame = elf::GameBaseT<RTSState, AI>;

    static constexpr int kFrameSkip = 50;

    MCTSActor() { }

    Response &evaluate(const RTSState &) {
        assert(game_.get());
        // Uniform distribution on resp.
        resp_.pi.resize(NUM_AISTATE);
        for (int i = 0; i < NUM_AISTATE; ++i) {
            resp_.pi[i] = make_pair(i, 1.0 / NUM_AISTATE);
        }
        resp_.value = 0.0;
        return resp_;
    }

    float reward(const RTSState &s) const {
        // Then we need to estimate the value. This is done by a random playout to the end.
        assert(game_.get());
        ai_->SpecifyNextAction(-1);

        RTSState s_dup = s;
        game_->SetState(&s_dup);
        game_->MainLoop();
        PlayerId winner = s_dup.env().GetWinnerId();

        float r = (winner == ai_->id()) ? 1.0 : 0.0;
        // cout << "reward: " << r << endl;
        return r;
    }

    bool forward(State &s, const Action &a) {
        assert(game_.get());
        ai_->SpecifyNextAction(a);
        game_->SetState(&s);
        auto res = game_->Step();
        return res == elf::GameResult::GAME_NORMAL;
    }

    string info() const {
        stringstream ss;
        ss << "Actor. ai = " << hex << ai_ << dec;
        return ss.str();
    }

    void SetId(PlayerId id) {
        AIOptions opt;
        vector<AI *> ais(2);
        ai_ = new FixedAI(opt, time(NULL));
        ais[0] = ai_;
        ais[1] = new SimpleAI(opt);

        if (id == 1) {
            swap(ais[0], ais[1]);
        }

        game_.reset(new RTSGame());
        for (AI * ai : ais) {
            game_->AddBot(ai, kFrameSkip);
        }
    }

protected:
    Response resp_;
    unique_ptr<RTSGame> game_;
    FixedAI *ai_;
};

class MCTSRTSAI : public AI {
public:
    MCTSRTSAI(const mcts::TSOptions &options)
        : mcts_ai_(options, [](int) { return new MCTSActor(); }) {
    }

    bool Act(const RTSState &s, RTSMCAction *a, const std::atomic_bool *done) override {
        a->Init(id(), name());
        int reduced_a;
        if (! mcts_ai_.Act(s, &reduced_a, done)) return false;
        cout << "MCTS [t=" << s.GetTick() << "] Action: " << (AIState)reduced_a << endl << flush;
        a->SetState9(reduced_a);
        return true;
    }

protected:
    void on_set_id() {
        mcts_ai_.SetId(id());
    }

private:
    elf::MCTSAI_T<MCTSActor> mcts_ai_;
};

