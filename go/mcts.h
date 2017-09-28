#pragma once

#include "elf/mcts.h"
#include "go_ai.h"

class MCTSGoState : public GoState {
public:
    using AI = DirectPredictAI;
    using Action = Coord;
    using State = GoState;

    MCTSGoState(const State &state) : State(state), value_(0) { }
    MCTSGoState() : value_(0) { }

    MCTSGoState &operator=(const State &state) {
        *((State *)this) = state;
        return *this;
    }

    bool evaluate_by_ai(AI *ai) {
        ai->SetState(*this);
        if (! ai->Act(0, nullptr, nullptr)) return false;
        ai->get_last_pi(&pi_);
        value_ = ai->get_last_value();
        return true;
    }

    const vector<pair<Action, float>> &pi() const { return pi_; }
    float value() const { return value_; }
    float reward() const { return value_; }

protected:
    vector<pair<Action, float>> pi_;
    float value_;
};

using MCTSGoAI = elf::MCTSAI_T<MCTSGoState>;

