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

#include "engine/rule_actor.h"

class MCRuleActor : public RuleActor {
public:
    std::map<AIState, UnitType> _state_build_map = {
        {STATE_BUILD_PEASANT, PEASANT},
        {STATE_BUILD_SWORDMAN, SWORDMAN},
        {STATE_BUILD_SPEARMAN, SPEARMAN},
        {STATE_BUILD_CAVALRY, CAVALRY},
        {STATE_BUILD_ARCHER, ARCHER},
        {STATE_BUILD_DRAGON, DRAGON},
        {STATE_BUILD_BARRACK, BARRACK},
        {STATE_BUILD_STABLE, STABLE},
        {STATE_BUILD_WORKSHOP, WORKSHOP},
        {STATE_BUILD_BLACKSMITH, BLACKSMITH},
        {STATE_BUILD_GUARD_TOWER, GUARD_TOWER},
        {STATE_BUILD_TOWN_HALL, TOWN_HALL},
    };
    MCRuleActor(){ }

    const Unit* GetTargetUnit(const GameEnv &env, const vector<vector<const Unit*> > &enemy_troops, const Player& player);

    // Act by a state array, used by MiniRTS
    bool ActByStateOld(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds);

    // Act by a state array, used by MiniRTS
    bool ActByState(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds);
    // Act by a state array for each unit, used by MiniRTS
    bool ActByState2(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds);
    // Act by a state array for each region, used by MiniRTS
    //bool ActByRegionalState(const GameEnv &env, const Reply &reply, string *state_string, AssignedCmds *assigned_cmds);

    // Determine state array for SimpleAI
    bool GetActSimpleState(vector<int>* state);
    // Determine state array for HitAndRunAI

    bool GetActTowerDefenseState(vector<int>* state);
    // [REGION_MAX_RANGE_X][REGION_MAX_RANGE_Y][REGION_RANGE_CHANNEL]
    bool ActWithMap(const GameEnv &env, const vector<vector<vector<int>>>& action_map, string *state_string, AssignedCmds *assigned_cmds);
};
#endif
