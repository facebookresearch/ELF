/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include "../engine/ai.h"
#include "python_options.h"
#include "mc_rule_actor.h"
#define NUM_RES_SLOT 5

using Comm = Context::Comm;
using AIComm = AICommT<Comm>;
using Data = typename AIComm::Data;

class AIBase : public AIWithComm<AIComm> {
protected:
    bool _respect_fow;

    // Feature extraction.
    void save_structured_state(const GameEnv &env, Data *data) const override;

public:
    AIBase() { }
    AIBase(PlayerId id, int frameskip, bool respect_fow, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : AIWithComm<AIComm>(id, frameskip, receiver, ai_comm), _respect_fow(respect_fow) {
    }
};

// TrainedAI2 for MiniRTS,  connected with a python wrapper / ELF.
class TrainedAI2 : public AIBase {
private:
    // Backup AI.
    // Used when we want the default ai to play for a while and then TrainedAI can take over.
    Tick _backup_ai_tick_thres;
    std::unique_ptr<AI> _backup_ai;
    MCRuleActor _mc_rule_actor;

protected:
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

    RuleActor *rule_actor() override { return &_mc_rule_actor; }

public:
    TrainedAI2(PlayerId id, int frame_skip, bool respect_fow, CmdReceiver *receiver, AIComm *ai_comm, AI *backup_ai = nullptr)
      : AIBase(id, frame_skip, respect_fow, receiver, ai_comm), _backup_ai_tick_thres(0) {
          if (ai_comm == nullptr) {
              throw std::range_error("TrainedAI2: ai_comm cannot be nullptr!");
          }
          if (backup_ai != nullptr) {
              backup_ai->SetId(GetId());
              backup_ai->SetCmdReceiver(_receiver);
              _backup_ai.reset(backup_ai);
          }
    }

    // Note that this is not thread-safe, so we need to be careful here.
    void SetBackupAIEndTick(Tick thres) { _backup_ai_tick_thres = thres; }
};

// Simple AI, rule-based AI for Mini-RTS
class SimpleAI : public AIBase {
private:
    MCRuleActor _mc_rule_actor;
    bool on_act(const GameEnv &env) override;
    RuleActor *rule_actor() override { return &_mc_rule_actor; }

public:
    SimpleAI() {
    }
    SimpleAI(PlayerId id, int frame_skip, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : AIBase(id, frame_skip, true, receiver, ai_comm)  {
    }

    SERIALIZER_DERIVED(SimpleAI, AIBase, _state);
};

// HitAndRun AI, rule-based AI for Mini-RTS
class HitAndRunAI : public AIBase {
private:
    MCRuleActor _mc_rule_actor;
    bool on_act(const GameEnv &env) override;
    RuleActor *rule_actor() override { return &_mc_rule_actor; }

public:
    HitAndRunAI() {
    }
    HitAndRunAI(PlayerId id, int frame_skip, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : AIBase(id, frame_skip, true, receiver, ai_comm) {
    }

    SERIALIZER_DERIVED(HitAndRunAI, AIBase, _state);
};
