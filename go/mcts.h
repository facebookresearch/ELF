#pragma once

#include "elf/mcts.h"
#include "go_ai.h"

class MCTSActor {
public:
    using AI = DirectPredictAI;
    using Action = Coord;
    using State = GoState;
    using NodeResponse = mcts::NodeResponseT<Action>;

    MCTSActor(AI *ai) {
        ai_ = ai;
        ai_->SetActorName("mcts_actor");
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

    string info() const { return string(); }

protected:
    NodeResponse resp_;
    AI *ai_ = nullptr;
};

using MCTSGoAI = elf::MCTSAIWithCommT<MCTSActor, AIComm>;

