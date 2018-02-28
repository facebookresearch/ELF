/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _MC_RULE_ACTOR_H_
#define _MC_RULE_ACTOR_H_

#include <map>
#include "engine/rule_actor.h"

class MCRuleActor : public RuleActor {
public:
     std::map<AIState, UnitType> _state_build_map = {
        {STATE_BUILD_WORKER, WORKER},
        {STATE_BUILD_SOLDIER, SOLDIER},
        {STATE_BUILD_TRUCK, TRUCK},
        {STATE_BUILD_TANK, TANK},
        {STATE_BUILD_CANNON, CANNON},
        {STATE_BUILD_FLIGHT, FLIGHT},
        {STATE_BUILD_BARRACK, BARRACK},
        {STATE_BUILD_HANGAR, HANGAR},
        {STATE_BUILD_WORKSHOP, WORKSHOP},
        {STATE_BUILD_FACTORY, FACTORY},
        {STATE_BUILD_DEFENSE_TOWER, DEFENSE_TOWER},
        {STATE_BUILD_BASE, BASE},
    };
    MCRuleActor(){ }

    const Unit* GetTargetUnit(const GameEnv &env, const vector<vector<const Unit*> > &enemy_troops, const Player& player);

    // Act by a state array, used by MiniRTS
    bool ActByState(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds);
    // Act by a state array for each unit, used by MiniRTS
    bool ActByState2(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds);
    // Act by a state array for each region, used by MiniRTS
    //bool ActByRegionalState(const GameEnv &env, const Reply &reply, string *state_string, AssignedCmds *assigned_cmds);

    // Determine state array for SimpleAI
    bool GetActSimpleState(vector<int>* state);
    // Determine state array for HitAndRunAI

    bool GetActHitAndRunState(vector<int>* state);
    // [REGION_MAX_RANGE_X][REGION_MAX_RANGE_Y][REGION_RANGE_CHANNEL]
    bool ActWithMap(const GameEnv &env, const vector<vector<vector<int>>>& action_map, string *state_string, AssignedCmds *assigned_cmds);
};
#endif
