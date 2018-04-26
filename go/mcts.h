/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "elf/mcts.h"
#include "go_ai.h"

class MCTSActor {
public:
    using Action = Coord;
    using State = GoState;
    using NodeResponse = mcts::NodeResponseT<Action>;

    MCTSActor(AIComm *ai_comm) {
        ai_.reset(new DirectPredictAI);
        ai_->InitAIComm(ai_comm);
        ai_->SetActorName("actor");
    }

    void set_ostream(ostream *oo) { oo_ = oo; }

    NodeResponse &evaluate(const GoState &s) {
        ai_->Act(s, nullptr, nullptr);
        ai_->get_last_pi(&resp_.pi, oo_);
        resp_.value = ai_->get_last_value();
        return resp_;
    }

    bool forward(GoState &s, Coord a) {
        return s.forward(a);
    }

    void SetId(int id) {
        ai_->SetId(id);
    }

    string info() const { return string(); }

protected:
    NodeResponse resp_;
    unique_ptr<DirectPredictAI> ai_;
    ostream *oo_ = nullptr;
};

using MCTSGoAI = elf::MCTSAIWithCommT<MCTSActor, AIComm>;

