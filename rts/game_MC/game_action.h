/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "engine/game_action.h"
#include "mc_rule_actor.h"

class RTSMCAction : public RTSAction {
private:
    enum ActionType { STATE9 = 0, SIMPLE = 1, HIT_AND_RUN = 2, CMD_INPUT = 3 };

public:
    RTSMCAction() : _type(CMD_INPUT), _action(-1) { }

    void SetState9(int action) {
        _type = STATE9;
        _action = action;
    }

    void SetSimpleAI() { _type = SIMPLE; }
    void SetHitAndRunAI() { _type = HIT_AND_RUN; }

    void SetUnitCmds(const std::vector<CmdInput> &unit_cmds) {
        _type = CMD_INPUT;
        _unit_cmds = unit_cmds;
    }

    bool Send(const GameEnv &env, CmdReceiver &receiver);

protected:
    ActionType _type;
    int _action;
    std::vector<CmdInput> _unit_cmds;
};

