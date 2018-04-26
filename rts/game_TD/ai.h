/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "engine/ai.h"
#include "python_options.h"
#include "td_rule_actor.h"

using Comm = Context::Comm;
using AIComm = AICommT<Comm>;
using Data = typename AIComm::Data;

class AIBase : public AIWithComm<AIComm> {
protected:
    float last_hp_level = 1.0;

    void on_save_data(Data *data) override {
        GameState *game = &data->newest();
        if (game->winner != INVALID) return;

        // assign partial rewards
        if (game->base_hp_level < last_hp_level) {
            game->last_r = game->base_hp_level - last_hp_level;
            last_hp_level = game->base_hp_level;
        }
    }
    // Feature extraction.
    void save_structured_state(const GameEnv &env, Data *data) const override;

public:
    AIBase() { }
    AIBase(const AIOptions &opt, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : AIWithComm<AIComm>(opt.name, opt.fs, receiver, ai_comm) {
    }
};

// TDTrainedAI for Tower Defense,  connected with a python wrapper / ELF.
class TDTrainedAI : public AIBase {
private:
    Tick _backup_ai_tick_thres;
    std::unique_ptr<AI> _backup_ai;
    TDRuleActor _td_rule_actor;
    RuleActor *rule_actor() override { return &_td_rule_actor; }
    bool on_act(const GameEnv &env) override;
    void on_set_id(PlayerId id) override {
        this->AIBase::on_set_id(id);
        if (_backup_ai != nullptr) _backup_ai->SetId(id);
    }

    void on_set_cmd_receiver(CmdReceiver *receiver) override {
        this->AIBase::on_set_cmd_receiver(receiver);
        if (_backup_ai != nullptr) _backup_ai->SetCmdReceiver(receiver);
    }

    void on_save_data(Data *data) override {
        this->AIBase::on_save_data(data);
        data->newest().ai_start_tick = _backup_ai_tick_thres;
    }

    bool need_structured_state(Tick tick) const override {
        if (_backup_ai != nullptr && tick < _backup_ai_tick_thres) {
            // We just use the backup AI.
            return false;
        } else {
            return true;
        }
    }
public:
    TDTrainedAI() {}
    TDTrainedAI(const AIOptions &opt, CmdReceiver *receiver, AIComm *ai_comm)
      : AIBase(opt, receiver, ai_comm) {
        if (ai_comm == nullptr) {
            throw std::range_error("TDTrainedAI: ai_comm cannot be nullptr!");
        }

    }
    void Reset() override {
        AIWithComm::Reset();
    }
    SERIALIZER_DERIVED(TDTrainedAI, AI, _state);
};

// TDSimple AI, rule-based AI for Tower Defense
class TDSimpleAI : public AIBase {
private:
    TDRuleActor _td_rule_actor;
    bool on_act(const GameEnv &env) override;
    RuleActor *rule_actor() override { return &_td_rule_actor; }
public:
    TDSimpleAI() {
    }
    TDSimpleAI(const AIOptions &opt, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
      : AIBase(opt, receiver, ai_comm) {
    }

    SERIALIZER_DERIVED(TDSimpleAI, AI, _state);
};

// TDBuiltInAI, environment for Tower Defense. i.e. generate waves to defeat.
class TDBuiltInAI : public AIBase {
private:
    TDRuleActor _td_rule_actor;
    bool on_act(const GameEnv &env) override;
    RuleActor *rule_actor() override { return &_td_rule_actor; }
public:
    TDBuiltInAI() {
    }
    TDBuiltInAI(const AIOptions &opt, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
      : AIBase(opt, receiver, ai_comm) {
    }

    SERIALIZER_DERIVED(TDBuiltInAI, AI, _state);
};
