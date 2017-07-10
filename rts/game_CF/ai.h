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
#include "cf_rule_actor.h"

// FlagTrainedAI for Capture the Flag,  connected with a python wrapper / ELF.
class FlagTrainedAI : public AI {
private:
    // Backup AI.
    // Used when we want the default ai to play for a while and then TrainedAI can take over.
    Tick _backup_ai_tick_thres;
    std::unique_ptr<AI> _backup_ai;
    CFRuleActor _cf_rule_actor;
    void set_rule_actor() override { _rule_actor = &_cf_rule_actor; }
    bool on_act(const GameEnv &env) override;
    void on_set_id(PlayerId id) override { if (_backup_ai != nullptr) _backup_ai->SetId(id); }
    void on_set_cmd_receiver(CmdReceiver *receiver) override { if (_backup_ai != nullptr) _backup_ai->SetCmdReceiver(receiver); }
    void on_save_data(ExtGame *game) const override { game->ai_start_tick = _backup_ai_tick_thres; }

public:
    FlagTrainedAI(PlayerId id, int frame_skip, CmdReceiver *receiver, AIComm *ai_comm, AI *backup_ai = nullptr)
      : AI(id, frame_skip, receiver, ai_comm), _backup_ai_tick_thres(0) {
          if (ai_comm == nullptr) {
              throw std::range_error("FlagTrainedAI: ai_comm cannot be nullptr!");
          }
          if (backup_ai != nullptr) {
              backup_ai->SetId(GetId());
              backup_ai->SetCmdReceiver(_receiver);
              _backup_ai.reset(backup_ai);
          }
          set_rule_actor();
          _rule_actor->SetReceiver(receiver);
          _rule_actor->SetPlayerId(id);
    }

    // Note that this is not thread-safe, so we need to be careful here.
    void SetBackupAIEndTick(Tick thres) {
        _backup_ai_tick_thres = thres;
    }

    bool NeedStructuredState(Tick tick) const override {
      if (_backup_ai != nullptr && tick < _backup_ai_tick_thres) {
          // We just use the backup AI.
          return false;
      } else {
          return true;
      }
    }
};

// FlagSimple AI, rule-based AI for Capture the Flag
class FlagSimpleAI : public AI {
private:
    bool on_act(const GameEnv &env) override;
    CFRuleActor _cf_rule_actor;
    void set_rule_actor() override { _rule_actor = &_cf_rule_actor; }

public:
    FlagSimpleAI() {
    }
    FlagSimpleAI(PlayerId id, int frame_skip, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : AI(id, frame_skip, receiver, ai_comm) {
          set_rule_actor();
          _rule_actor->SetReceiver(receiver);
          _rule_actor->SetPlayerId(id);
    }

    SERIALIZER_DERIVED(FlagSimpleAI, AI, _state);
};
