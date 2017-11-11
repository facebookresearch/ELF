#pragma once

#include "elf/mcts.h"
#include "go_ai.h"
#include <iostream>

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

    float reward(const GoState &s, float value) const {
        // Compute value of the current situation, if the game._ply has passed a threshold.
        // if (s.GetPly() <= BOARD_SIZE * BOARD_SIZE) return value;

        // If the game last too long, then we can formally use the evaluation.
        std::mt19937 rng(time(NULL));
        auto func = [&]() -> unsigned int { return rng(); };
        return s.Evaluate(func);
    }

protected:
    NodeResponse resp_;
    unique_ptr<DirectPredictAI> ai_;
    ostream *oo_ = nullptr;
};

using MCTSGoAI = elf::MCTSAIWithCommT<MCTSActor, AIComm>;

