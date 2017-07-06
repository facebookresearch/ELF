/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "ai.h"

bool TDTrainedAI::on_act(const GameEnv &env) {
    // Get the current action from the queue.
    int h = 0;
    const Reply& reply = _ai_comm->newest().reply;
    if (reply.global_action >= 0) {
        // action
        h = reply.global_action;
    }
    return gather_decide(env, [&](const GameEnv &e, string*, AssignedCmds *assigned_cmds) {
        return _td_rule_actor.TowerDefenseActByState(e, h, assigned_cmds);
    });
}

bool TDSimpleAI::on_act(const GameEnv &env) {
    return gather_decide(env, [&](const GameEnv &e, string*, AssignedCmds *assigned_cmds) {
        return _td_rule_actor.ActTowerDefenseSimple(e, assigned_cmds);
    });
}

bool TDBuiltInAI::on_act(const GameEnv &env) {
    return gather_decide(env, [&](const GameEnv &e, string*, AssignedCmds *assigned_cmds) {
        return _td_rule_actor.ActTowerDefenseBuiltIn(e, assigned_cmds);
    });
}
