/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include "engine/ai.h"
#include "python_options.h"
#include "cf_rule_actor.h"
#define NUM_RES_SLOT 5

using Comm = Context::Comm;
using AIComm = AICommT<Comm>;
using Data = typename AIComm::Data;

class AIBase : public AIWithComm<AIComm> {
protected:
    int last_r0 = 0;
    int last_r1 = 0;
    int last_x = 9;

    void on_save_data(Data *data) override {
        GameState *game = &data->newest();
        if (game->winner != INVALID) return;

        // assign partial rewards
        if (game->r0 > last_r0) {
            game->last_r = 0.2;
            last_r0 = game->r0;
        } else if (game->r1 > last_r1) {
            game->last_r = -0.2;
            last_r1 = game->r1;
        } else {
            game->last_r = float(last_x - game->flag_x) / 100;
            last_x = game->flag_x;
        }
    }

    // Feature extraction.
    void save_structured_state(const GameEnv &env, Data *data) const override;

public:
    AIBase() { }
    AIBase(PlayerId id, int frameskip, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : AIWithComm<AIComm>(id, frameskip, receiver, ai_comm) {
    }
};

// FlagTrainedAI for Capture the Flag,  connected with a python wrapper / ELF.
class FlagTrainedAI : public AIBase {
private:
    // Backup AI.
    // Used when we want the default ai to play for a while and then TrainedAI can take over.
    Tick _backup_ai_tick_thres;
    std::unique_ptr<AI> _backup_ai;
    CFRuleActor _cf_rule_actor;

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

    RuleActor *rule_actor() override { return &_cf_rule_actor; }

public:
    FlagTrainedAI(PlayerId id, int frame_skip, CmdReceiver *receiver, AIComm *ai_comm, AI *backup_ai = nullptr)
      : AIBase(id, frame_skip, receiver, ai_comm), _backup_ai_tick_thres(0) {
          if (ai_comm == nullptr) {
              throw std::range_error("FlagTrainedAI: ai_comm cannot be nullptr!");
          }
          if (backup_ai != nullptr) {
              backup_ai->SetId(GetId());
              backup_ai->SetCmdReceiver(_receiver);
              _backup_ai.reset(backup_ai);
          }
    }
    // Note that this is not thread-safe, so we need to be careful here.
    void SetBackupAIEndTick(Tick thres) {
        _backup_ai_tick_thres = thres;
    }
};

// FlagSimple AI, rule-based AI for Capture the Flag
class FlagSimpleAI : public AIBase {
private:
    bool on_act(const GameEnv &env) override;
    CFRuleActor _cf_rule_actor;

    RuleActor *rule_actor() override { return &_cf_rule_actor; }
public:
    FlagSimpleAI() {
    }
    FlagSimpleAI(PlayerId id, int frame_skip, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : AIBase(id, frame_skip, receiver, ai_comm) {
    }

    SERIALIZER_DERIVED(FlagSimpleAI, AI, _state);
};
