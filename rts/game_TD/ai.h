/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include "../engine/omni_ai.h"
#include "td_rule_actor.h"

// TDTrainedAI for Tower Defense,  connected with a python wrapper / ELF.
class TDTrainedAI : public OmniAI {
private:
    Tick _backup_ai_tick_thres;
    std::unique_ptr<OmniAI> _backup_ai;
    TDRuleActor _td_rule_actor;
    void set_rule_actor() override { _rule_actor = &_td_rule_actor; }
    bool on_act(const GameEnv &env) override;
public:
    TDTrainedAI() {}
    TDTrainedAI(PlayerId id, int frame_skip, CmdReceiver *receiver, AIComm *ai_comm, OmniAI *backup_ai = nullptr)
      : OmniAI(id, frame_skip, receiver, ai_comm),  _backup_ai_tick_thres(0){
        if (ai_comm == nullptr) {
            throw std::range_error("TDTrainedAI: ai_comm cannot be nullptr!");
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

    void SetBackupAIEndTick(Tick thres) {
        _backup_ai_tick_thres = thres;
    }
    SERIALIZER_DERIVED(TDTrainedAI, OmniAI, _state);
};

// TDSimple AI, rule-based AI for Tower Defense
class TDSimpleAI : public OmniAI {
private:
    TDRuleActor _td_rule_actor;
    void set_rule_actor() override { _rule_actor = &_td_rule_actor; }
    bool on_act(const GameEnv &env) override;

public:
    TDSimpleAI() {
    }
    TDSimpleAI(PlayerId id, int frame_skip, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : OmniAI(id, frame_skip, receiver, ai_comm) {
          set_rule_actor();
          _rule_actor->SetReceiver(receiver);
          _rule_actor->SetPlayerId(id);
    }

    SERIALIZER_DERIVED(TDSimpleAI, OmniAI, _state);
};

// TDBuiltInAI, environment for Tower Defense. i.e. generate waves to defeat.
class TDBuiltInAI : public OmniAI {
private:
    TDRuleActor _td_rule_actor;
    void set_rule_actor() override { _rule_actor = &_td_rule_actor; }
    bool on_act(const GameEnv &env) override;

public:
    TDBuiltInAI() {
    }
    TDBuiltInAI(PlayerId id, int frame_skip, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : OmniAI(id, frame_skip, receiver, ai_comm) {
          set_rule_actor();
          _rule_actor->SetReceiver(receiver);
          _rule_actor->SetPlayerId(id);
    }

    SERIALIZER_DERIVED(TDBuiltInAI, OmniAI, _state);
};
