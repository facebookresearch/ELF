/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "cf_rule_actor.h"

bool CFRuleActor::FlagActByState(const GameEnv &env, const vector<int>& state, AssignedCmds *assigned_cmds) {
    const auto &enemy_troops = _preload.EnemyTroops();
    const auto &my_troops = _preload.MyTroops();

    if (state[FLAGSTATE_GET_FLAG] == 1) {
        if (!enemy_troops[FLAG].empty()) {
            // flag is idle, get it
            UnitId flag_id = enemy_troops[FLAG][0]->GetId();
            for (const Unit *u : my_troops[FLAG_ATHLETE]) {
                store_cmd(u, CmdBPtr(new CmdGetFlag(INVALID, flag_id)), assigned_cmds);
            }
        }
    }
    if (state[FLAGSTATE_ATTACK_FLAG] == 1) {
        for (const Unit *eu : enemy_troops[FLAG_ATHLETE]) {
            if (eu->HasFlag()) {
                for (const Unit *u : my_troops[FLAG_ATHLETE]) {
                    store_cmd(u, _A(eu->GetId()), assigned_cmds);
                }
            }
        }
    }
    if (state[FLAGSTATE_ESCORT_FLAG] == 1) {
        for (const Unit *u : my_troops[FLAG_ATHLETE]) {
            if (u->HasFlag()) {
                // try to send the flag back to base
                store_cmd(u, CmdBPtr(new CmdEscortFlagToBase(INVALID)), assigned_cmds);
            }
        }
    }
    if (state[FLAGSTATE_PROTECT_FLAG] == 1) {
        UnitId our_flag_athlete_id = INVALID;
        for (const Unit *u : my_troops[FLAG_ATHLETE]) {
            if (u->HasFlag()) {
                our_flag_athlete_id = u->GetId();
            }
        }
        // others, protect our flag athlete
        const Unit* our_flag_athlete = env.GetUnit(our_flag_athlete_id);
        if (our_flag_athlete != nullptr) {
            UnitId damage_from = our_flag_athlete->GetProperty().GetLastDamageFrom();
            const Unit *source = env.GetUnit(damage_from);
            if (source != nullptr) {
                for (const Unit *u : my_troops[FLAG_ATHLETE]) {
                    if (!u->HasFlag()) {
                        store_cmd(u, _A(damage_from), assigned_cmds);
                    }
                }
            }
        }
    }
    /*
    if (state[FLAGSTATE_MOVE] == 1) {
        // move towards center of the map
        for (const Unit *u : my_troops[FLAG_ATHLETE]) {
            store_cmd(u, _M(PointF(9.5, 9.5)), assigned_cmds);
        }
    }
    if (state[FLAGSTATE_ATTACK] == 1) {
        // don't care about flag, just attack enemy
        if (!enemy_troops[FLAG_ATHLETE].empty()) {
            UnitId target = enemy_troops[FLAG_ATHLETE][0]->GetId();
            for (const Unit *u : my_troops[FLAG_ATHLETE]) {
                store_cmd(u, _A(target), assigned_cmds);
            }
        }
    }*/
    return true;
}

bool CFRuleActor::GetFlagActSimpleState(const GameEnv &env, vector<int>* state) {
    vector<int> &_state = *state;
    const auto &enemy_troops = _preload.EnemyTroops();
    const auto &my_troops = _preload.MyTroops();

    if (! enemy_troops[FLAG].empty()) {
        _state[FLAGSTATE_GET_FLAG] = 1;
    } else {
        UnitId our_flag_athlete_id = INVALID;
        for (const Unit *u : my_troops[FLAG_ATHLETE]) {
            if (u->HasFlag()) {
                our_flag_athlete_id = u->GetId();
            }
        }
        // others, protect our flag athlete
        const Unit* our_flag_athlete = env.GetUnit(our_flag_athlete_id);
        if (our_flag_athlete != nullptr) {
            _state[FLAGSTATE_ESCORT_FLAG] = 1;
            _state[FLAGSTATE_PROTECT_FLAG] = 1;
        } else {
            _state[FLAGSTATE_ATTACK_FLAG] = 1;
        }
    }
    return true;
}
