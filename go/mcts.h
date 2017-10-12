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

    NodeResponse &evaluate(const GoState &s) {
        ai_->Act(s, nullptr, nullptr);
        ai_->get_last_pi(&resp_.pi);
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
};

using MCTSGoAI = elf::MCTSAIWithCommT<MCTSActor, AIComm>;

