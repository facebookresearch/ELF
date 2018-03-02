/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "td_rule_actor.h"

bool TDRuleActor::TowerDefenseActByState(const GameEnv &env, int state, AssignedCmds *assigned_cmds) {
    const auto &enemy_troops = _preload.EnemyTroops();
    const auto &my_troops = _preload.MyTroops();
    int tower_price = env.GetGameDef().unit(TOWER).GetUnitCost();
    if (my_troops[TOWER_TOWN_HALL].empty()) {
        return false;
    }
    const Unit *base = my_troops[TOWER_TOWN_HALL][0];
    if (_preload.Resource() >= tower_price) {
        int x = state / 20;
        int y = state % 20;
        store_cmd(base, CmdDPtr(new CmdBuildTower(INVALID, PointF(x, y), tower_price, _player_id)), assigned_cmds);
    }
    auto targets = enemy_troops[TOWER_ATTACKER];
    for (const Unit *u : my_troops[TOWER]) {
        float closest = std::numeric_limits<float>::max();
        const Unit *closest_target = closest_dist(targets, u->GetPointF(), &closest);
        if (closest_target != nullptr && closest <= u->GetProperty()._att_r) {
            UnitId opponent_target_id = closest_target->GetId();
            store_cmd(u, _A(opponent_target_id), assigned_cmds);
        }
    }
    return true;
}

bool TDRuleActor::ActTowerDefenseSimple(const GameEnv &env, AssignedCmds *assigned_cmds) {
    const auto &enemy_troops = _preload.EnemyTroops();
    const auto &my_troops = _preload.MyTroops();
    int tower_price = env.GetGameDef().unit(TOWER).GetUnitCost();
    if (my_troops[TOWER_TOWN_HALL].empty()) {
        return false;
    }
    const Unit *base = my_troops[TOWER_TOWN_HALL][0];
    if (_preload.Resource() >= tower_price) {
        PointF p;
        if (env.FindBuildPlaceNearby(PointF(9, 9), 2, &p) && ! p.IsInvalid()) {
            store_cmd(base, CmdDPtr(new CmdBuildTower(INVALID, p, tower_price, _player_id)), assigned_cmds);
        }
    }
    auto targets = enemy_troops[TOWER_ATTACKER];
    for (const Unit *u : my_troops[TOWER]) {
        float closest = std::numeric_limits<float>::max();
        const Unit *closest_target = closest_dist(targets, u->GetPointF(), &closest);
        if (closest_target != nullptr && closest <= u->GetProperty()._att_r) {
            UnitId opponent_target_id = closest_target->GetId();
            store_cmd(u, _A(opponent_target_id), assigned_cmds);
        }
    }
    return true;
}

bool TDRuleActor::ActTowerDefenseBuiltIn(const GameEnv&, AssignedCmds *assigned_cmds) {
    const auto &enemy_troops = _preload.EnemyTroops();
    const auto &my_troops = _preload.MyTroops();

    if (my_troops[TOWER_TOWN_HALL].empty()){
        return false;
    }
    const Unit *base = my_troops[TOWER_TOWN_HALL][0];
    const int ticks_per_wave = 200;
    int tick = _receiver->GetTick();
    if (tick % ticks_per_wave == 0) {
        store_cmd(base, CmdIPtr(new CmdTowerDefenseWaveStart(INVALID, int(tick / ticks_per_wave))), assigned_cmds);
    }
    if (! enemy_troops[TOWER_TOWN_HALL].empty()) {
        UnitId base_id = enemy_troops[TOWER_TOWN_HALL][0]->GetId();
        for (const Unit *u : my_troops[TOWER_ATTACKER]) {
            store_cmd(u, _A(base_id), assigned_cmds);
        }
    }
    return true;
}
