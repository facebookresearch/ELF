/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#ifndef _CF_RULE_ACTOR_H_
#define _CF_RULE_ACTOR_H_

#include "engine/rule_actor.h"
#include "engine/cmd.h"
#include "cmd_specific.gen.h"

class CFRuleActor : public RuleActor {
public:
    CFRuleActor(){ }
    // Act by a state array, used by Capture the flag
    bool FlagActByState(const GameEnv &env, const vector<int>& state, AssignedCmds *assigned_cmds);
    // Determine state array for FlagSimpleAI
    bool GetFlagActSimpleState(const GameEnv &env, vector<int>* state);
};
#endif
