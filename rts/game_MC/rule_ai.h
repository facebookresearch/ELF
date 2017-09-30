#pragma once

#include "ai.h"

// Simple AI, rule-based AI for Mini-RTS
class SimpleAI : public AI {
public:
    SimpleAI(const AIOptions &opt) : AI(opt.name)  { }

    // SERIALIZER_DERIVED(SimpleAI, AIBase, _state);

private:
    bool Act(const RTSState &, RTSMCAction *action, const atomic_bool *) override {
        action->Init(id(), name());
        action->SetSimpleAI();
        return true;
    }
};

// HitAndRun AI, rule-based AI for Mini-RTS
class HitAndRunAI : public AI {
public:
    HitAndRunAI(const AIOptions &opt) : AI(opt.name)  { }

    // SERIALIZER_DERIVED(HitAndRunAI, AIBase, _state);

private:
    bool Act(const RTSState &, RTSMCAction *action, const atomic_bool *) override {
        action->Init(id(), name());
        action->SetHitAndRunAI();
        return true;
    }
};


