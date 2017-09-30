#pragma once

#include "elf/mcts.h"
#include "go_ai.h"

class MCTSGoState : public GoState {
public:
    using AI = DirectPredictAI;
    using Action = Coord;
    using State = GoState;

    MCTSGoState &operator=(const State &state) {
        *((State *)this) = state;
        return *this;
    }

    void SetAI(AI *ai) { ai_ = ai; }

    bool evaluate() {
        if (! ai_->Act(*this, nullptr, nullptr)) return false;
        ai_->get_last_pi(&pi_);
        value_ = ai_->get_last_value();
        return true;
    }

    const vector<pair<Action, float>> &pi() const { return pi_; }
    float value() const { return value_; }
    float reward() const { return value_; }

protected:
    vector<pair<Action, float>> pi_;
    float value_ = 0.0;
    AI *ai_ = nullptr;
};

using MCTSGoAI = elf::MCTSAI_T<MCTSGoState>;

