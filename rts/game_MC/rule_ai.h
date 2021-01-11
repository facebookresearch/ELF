/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "ai.h"

// Simple AI, rule-based AI for Mini-RTS
class SimpleAI : public AI {
public:
    SimpleAI(const AIOptions &opt) : AI(opt.name)  { }

    // SERIALIZER_DERIVED(SimpleAI, AIBase, _state);

private:
    bool Act(const RTSState &, RTSMCAction *action, const atomic_bool *) override {
        //cout<<"SimpleAI Acting"<<endl;
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

class RandomAI : public AI {
public:
    RandomAI(int seed) : AI("random"), rng_(seed)  { }

    // SERIALIZER_DERIVED(HitAndRunAI, AIBase, _state);

private:
    std::mt19937 rng_;
    bool Act(const RTSState &, RTSMCAction *action, const atomic_bool *) override {
        action->Init(id(), name());
        action->SetState9(rng_() % NUM_AISTATE);
        return true;
    }
};

class FixedAI : public AI {
public:
    FixedAI(const AIOptions &opt, int seed) : AI(opt.name), rng_(seed) { }

    void SpecifyNextAction(int a) {
        specified_action_ = a;
    }

    // SERIALIZER_DERIVED(HitAndRunAI, AIBase, _state);

private:
    int specified_action_ = -1;
    std::mt19937 rng_;

    bool Act(const RTSState &, RTSMCAction *action, const atomic_bool *) override {
        action->Init(id(), name());
        int a = specified_action_;
        if (a < 0) a = rng_() % NUM_AISTATE;
        action->SetState9(a);
        return true;
    }
};

