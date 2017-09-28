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

    reduced::PredictAI predict;
    reduced::ForwardAI forward;
    reduced::ProjectAI project;
};

class MCTSReducedState {
public:
    using AI = MultiAI;
    using Action = int;
    using State = MCTSReducedState;
    using FullState = RTSState;

    void SetAI(AI *ai) { ai_ = ai; }

    MCTSReducedState &operator=(const FullState &state) {
        auto &ai = ai_->project;
        ai.ActImmediate(state, &s_.state);
        return *this;
    }

    bool evaluate() {
        auto &ai = ai_->predict;
        if (! ai.ActImmediate(s_.state, &pred_)) return false;
        return true;
    }

    bool forward(const Action &a) {
        s_.action = a;
        auto &ai = ai_->forward;
        if (! ai.ActImmediate(s_, &s_.state)) return false;
        return true;
    }

    const vector<pair<Action, float>> &pi() const { return pred_.pi; }
    float value() const { return pred_.value; }
    float reward() const { return pred_.value; }

protected:
    ReducedState s_;
    ReducedPred pred_;
    AI *ai_ = nullptr;
};

// using MCTSRTSAI = elf::MCTSAI_T<MCTSState>;
using MCTSRTSReducedAI = elf::MCTSAI_Embed_T<RTSState, RTSMCAction, MCTSReducedState>;

