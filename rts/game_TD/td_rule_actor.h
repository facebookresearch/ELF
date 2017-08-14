/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _TD_RULE_ACTOR_H_
#define _TD_RULE_ACTOR_H_

#include "engine/rule_actor.h"
#include "engine/cmd.h"
#include "cmd_specific.gen.h"

class TDRuleActor : public RuleActor {
public:
    TDRuleActor(){ }
    // Act by a state array, used by Capture the flag    // Act by a state array, used by Tower defense
    bool TowerDefenseActByState(const GameEnv &env, int state, AssignedCmds *assigned_cmds);
    // Determine state array for TowerDefenseSimpleAI
    bool ActTowerDefenseSimple(const GameEnv &env, AssignedCmds *assigned_cmds);
    // Built-In action for Tower Defense environment.
    bool ActTowerDefenseBuiltIn(const GameEnv&, AssignedCmds *assigned_cmds);
};
#endif
