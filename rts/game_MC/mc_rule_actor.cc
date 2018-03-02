/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "mc_rule_actor.h"

bool MCRuleActor::ActByState2(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds) {
    assigned_cmds->clear();
    *state_string = "";

    RegionHist hist;

    // Then loop over all my troops to run.
    const auto& all_my_troops = _preload.AllMyTroops();
    for (const Unit *u : all_my_troops) {
        // Get the bin id.
        act_per_unit(env, u, &state[0], &hist, state_string, assigned_cmds);
    }

    return true;
}
const Unit* MCRuleActor::GetTargetUnit(const GameEnv &env, const vector<vector<const Unit*> > &enemy_troops, const Player& player){
    const vector<UnitType> &attack_order = {TOWN_HALL, GUARD_TOWER, BLACKSMITH, BARRACK, STABLES, WORKSHOP};
    for (const UnitType &target_type : attack_order) {
        const auto& target_units = enemy_troops[target_type];
        if (!target_units.empty()) {
            for (const Unit *u : target_units) {
                // cout << "potential unit " << u->GetUnitType() << " at " << u->GetPointF();
                if (player.FilterWithFOW(*u)) {
                    return u;
                }
                Loc loc = env.GetMap().GetLoc(u->GetPointF().ToCoord());
                const Fog &f = player.GetFog(loc);
                if (!f.seen_units().empty()) {
                    return u;
                }
            }
        }
    }
    return nullptr;
}

bool MCRuleActor::ActByState(const GameEnv &env, const vector<int>& state, string* /*state_string*/, AssignedCmds *assigned_cmds) {
    const GameDef& gamedef = env.GetGameDef();
    const auto& my_troops = _preload.MyTroops();
    const auto& enemy_troops = _preload.EnemyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    const auto& all_my_troops = _preload.AllMyTroops();
    const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();
    const Player& player = env.GetPlayer(_player_id);
    // cout << "entering act by state" << endl << flush;
    if (state[STATE_GATHER]) {
        for (const Unit *u : my_troops[PEASANT]) {
            if (IsIdle(*_receiver, *u)) {
                // Gather!
                float dist, closest_dist=1e10;
                UnitId base_id, resource_id;
                for (const Unit* resource : my_troops[RESOURCE]) {
                    // check closest resource
                    PointF resource_loc = resource->GetPointF();
                    UnitId closest_base_id = env.FindClosestBase(_player_id, resource_loc, &dist);
                    if (dist < closest_dist) {
                        closest_dist = dist;
                        base_id = closest_base_id;
                        resource_id = resource->GetId();
                    }
                }
                store_cmd(u, _G(base_id, resource_id), assigned_cmds);
            }
        }
    }
    //cout << "after gather" << endl << flush;
    for (int build_state = (int)STATE_BUILD_PEASANT; build_state < (int)STATE_BUILD_TOWN_HALL; build_state++) {
        if (state[build_state]) {
            //cout << "in build state " << build_state << " " << (AIState)build_state << endl << flush;
            UnitType& build_unit = _state_build_map[(AIState)build_state];
            if (_preload.BuildIfAffordable(build_unit)) {
                //cout << "building " << build_unit << endl << flush;
                const UnitType& build_from = gamedef.unit(build_unit)._build_from;
                //cout << "building from " << build_from << endl << flush;
                if (env.GetGameDef().IsUnitTypeBuilding(build_from)) {
                    //cout << "build from building" << endl << flush;
                    const Unit *u = GameEnv::PickFirstIdle(my_troops[build_from], *_receiver);
                    if (u != nullptr) {
                        store_cmd(u, _B_CURR_LOC(build_unit), assigned_cmds);
                    }
                } else {
                    //cout << "build from worker" << endl << flush;
                    const Unit *u = GameEnv::PickIdleOrGather(my_troops[PEASANT], *_receiver);
                    if (u != nullptr) {
                        const UnitTemplate& unit_def = gamedef.unit(u->GetUnitType());
                        PointF p;
                        if (env.FindEmptyPlaceNearby(unit_def, _preload.BaseLoc(), 3, &p) && ! p.IsInvalid())
                            store_cmd(u, _B(build_unit, p), assigned_cmds);
                    }
                }
            }
        }
    }
    //cout << "after build" << endl << flush;
    if (state[STATE_BUILD_TOWN_HALL]) {
        if (_preload.BuildIfAffordable(TOWN_HALL)) {
            for (const Unit* resource : enemy_troops[RESOURCE]) {
                // check if there is a unit around resource
                PointF resource_loc = resource->GetPointF();
                UnitId closest_id = env.FindClosestEnemy(2, resource_loc, 3);
                if (closest_id == INVALID) {
                    const Unit *u = GameEnv::PickIdleOrGather(my_troops[PEASANT], *_receiver);
                    PointF p;
                    const UnitTemplate& unit_def = gamedef.unit(u->GetUnitType());
                    if (env.FindEmptyPlaceNearby(unit_def, resource_loc, 3, &p) && ! p.IsInvalid())
                        store_cmd(u, _B(TOWN_HALL, p), assigned_cmds);
                }
            }
        }
    }
    //cout << "after build base" << endl << flush;
    if (state[STATE_SCOUT]) {
        const auto& map = env.GetMap();
        const int margin = 4;
        const vector<int> &xs = {margin, margin, map.GetXSize() - margin, map.GetXSize() - margin};
        const vector<int> &ys = {margin, map.GetYSize() - margin, margin, map.GetYSize() - margin};
        for (size_t i = 0; i < xs.size(); i++) {
            Coord c = Coord(xs[i], ys[i]);
            Loc loc = map.GetLoc(c);
            const Fog &f = player.GetFog(loc);
            if (!f.HasSeenTerrain()) {
                // Send a unit to scout, in this order
                const vector<UnitType> &scout_order = {DRAGON, SPEARMAN, CAVALRY, SWORDMAN, ARCHER, PEASANT};
                for (const UnitType &target_type : scout_order) {
                    const auto& target_units = my_troops[target_type];
                    if (!target_units.empty()) {
                        store_cmd(target_units[0], _M(PointF(c)), assigned_cmds);
                        break;
                    }
                }
            }
        }

    }
    //cout << "after scout" << endl << flush;
    if (state[STATE_ATTACK]) {
        const Unit* target_unit = GetTargetUnit(env, enemy_troops, player);
        if (target_unit != nullptr) {
            auto cmd = _A(target_unit->GetId());
            for (const Unit *u : all_my_troops) {
                const CmdDurative *curr_cmd = GetCurrCmd(*_receiver, *u);
                if (curr_cmd == nullptr && u->GetUnitType() != PEASANT) {
                    store_cmd(u, cmd->clone(), assigned_cmds);
                }
            }
        }
    }

    if (state[STATE_ATTACK_IN_RANGE]) {
      if (! enemy_troops_in_range.empty()) {
        auto cmd = _A(enemy_troops_in_range[0]->GetId());
        for (const Unit *u : all_my_troops) {
            const CmdDurative *curr_cmd = GetCurrCmd(*_receiver, *u);
            if (curr_cmd == nullptr && u->GetUnitType() != PEASANT) {
                store_cmd(u, cmd->clone(), assigned_cmds);
            }
        }
      }
    }
    //cout << "after attack" << endl << flush;

    if (state[STATE_DEFEND]) {
      // Group Retaliation. All troops attack.
      const Unit *enemy_at_resource = _preload.EnemyAtResource();
      if (enemy_at_resource != nullptr) {
          batch_store_cmds(all_my_troops, _A(enemy_at_resource->GetId()), true, assigned_cmds);
      }

      const Unit *enemy_at_base = _preload.EnemyAtBase();
      if (enemy_at_base != nullptr) {
          batch_store_cmds(all_my_troops, _A(enemy_at_base->GetId()), true, assigned_cmds);
      }

      if (! enemy_attacking_economy.empty()) {
        auto it = enemy_attacking_economy.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds(all_my_troops, cmd, true, assigned_cmds);
      }
    }
    //cout << "after defend" << endl << flush;
    return true;
}


bool MCRuleActor::ActByStateOld(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds) {
    // Each unit can only have one command. So we have this map.
    // cout << "Enter ActByState" << endl << flush;
    assigned_cmds->clear();
    *state_string = "NOOP";

    // Build workers.
    if (state[STATE_BUILD_PEASANT]) {
        *state_string = "Build worker..NOOP";
        const Unit *base = _preload.Base();
        if (IsIdle(*_receiver, *base)) {
            if (_preload.BuildIfAffordable(PEASANT)) {
                *state_string = "Build worker..Success";
                store_cmd(base, _B_CURR_LOC(PEASANT), assigned_cmds);
            }
        }
    }

    const auto& my_troops = _preload.MyTroops();

    // Ask workers to gather.
    for (const Unit *u : my_troops[PEASANT]) {
        if (IsIdle(*_receiver, *u)) {
            // Gather!
            store_cmd(u, _preload.GetGatherCmd(), assigned_cmds);
        }
    }

    // If resource permit, build barracks.
    if (state[STATE_BUILD_BARRACK]) {
        *state_string = "Build barracks..NOOP";
        if (_preload.Affordable(BARRACK)) {
            // cout << "Building barracks!" << endl;
            const Unit *u = GameEnv::PickIdleOrGather(my_troops[PEASANT], *_receiver);
            if (u != nullptr) {
                CmdBPtr cmd = _preload.GetBuildBarracksCmd(env, *u);
                if (cmd != nullptr) {
                    *state_string = "Build barracks..Success";
                    store_cmd(u, std::move(cmd), assigned_cmds);
                    _preload.Build(BARRACK);
                }
            }
        }
    }

    // If we have barracks with resource, build troops.
    if (state[STATE_BUILD_MELEE_TROOP]) {
        *state_string = "Build Melee Troop..NOOP";
        if (_preload.Affordable(SPEARMAN)) {
            const Unit *u = GameEnv::PickFirstIdle(my_troops[BARRACK], *_receiver);
            if (u != nullptr) {
                *state_string = "Build Melee Troop..Success";
                store_cmd(u, _B_CURR_LOC(SPEARMAN), assigned_cmds);
                _preload.Build(SPEARMAN);
            }
        }
    }

    if (state[STATE_BUILD_RANGE_TROOP]) {
        *state_string = "Build Range Troop..NOOP";
        if (_preload.Affordable(CAVALRY)) {
            const Unit *u = GameEnv::PickFirstIdle(my_troops[BARRACK], *_receiver);
            if (u != nullptr) {
                *state_string = "Build Range Troop..Success";
                store_cmd(u, _B_CURR_LOC(CAVALRY), assigned_cmds);
                _preload.Build(CAVALRY);
            }
        }
    }

    if (state[STATE_ATTACK]) {
        *state_string = "Attack..Normal";
        // Then let's go and fight.
        auto cmd = _preload.GetAttackEnemyBaseCmd();
        batch_store_cmds(my_troops[SPEARMAN], cmd, false, assigned_cmds);
        batch_store_cmds(my_troops[CAVALRY], cmd, false, assigned_cmds);
    }

    const auto& enemy_troops = _preload.EnemyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    const auto& all_my_troops = _preload.AllMyTroops();
    const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();

    if (state[STATE_HIT_AND_RUN]) {
        *state_string = "Hit and run";
        // cout << "Enter hit and run procedure" << endl << flush;
        if (enemy_troops[SPEARMAN].empty() && enemy_troops[CAVALRY].empty() && ! enemy_troops[PEASANT].empty()) {
            // cout << "Enemy only have worker" << endl << flush;
            for (const Unit *u : my_troops[CAVALRY]) {
                hit_and_run(env, u, enemy_troops[PEASANT], assigned_cmds);
            }
        }
        if (! enemy_troops[SPEARMAN].empty()) {
            // cout << "Enemy only have malee attacker" << endl << flush;
            for (const Unit *u : my_troops[CAVALRY]) {
                hit_and_run(env, u, enemy_troops[SPEARMAN], assigned_cmds);
            }
        }
        if (! enemy_troops[CAVALRY].empty()) {
            auto cmd = _A(enemy_troops[CAVALRY][0]->GetId());
            batch_store_cmds(my_troops[SPEARMAN], cmd, false, assigned_cmds);
            batch_store_cmds(my_troops[CAVALRY], cmd, false, assigned_cmds);
        }
    }

    if (state[STATE_ATTACK_IN_RANGE]) {
      *state_string = "Attack enemy in range..NOOP";
      if (! enemy_troops_in_range.empty()) {
        *state_string = "Attack enemy in range..Success";
        auto cmd = _A(enemy_troops_in_range[0]->GetId());
        batch_store_cmds(my_troops[SPEARMAN], cmd, false, assigned_cmds);
        batch_store_cmds(my_troops[CAVALRY], cmd, false, assigned_cmds);
      }
    }

    if (state[STATE_DEFEND]) {
      // Group Retaliation. All troops attack.
      *state_string = "Defend enemy attack..NOOP";

      const Unit *enemy_at_resource = _preload.EnemyAtResource();
      if (enemy_at_resource != nullptr) {
          *state_string = "Defend enemy attack..Success";
          batch_store_cmds(all_my_troops, _A(enemy_at_resource->GetId()), true, assigned_cmds);
      }

      const Unit *enemy_at_base = _preload.EnemyAtBase();
      if (enemy_at_base != nullptr) {
          *state_string = "Defend enemy attack..Success";
          batch_store_cmds(all_my_troops, _A(enemy_at_base->GetId()), true, assigned_cmds);
      }

      if (! enemy_attacking_economy.empty()) {
        *state_string = "Defend enemy attack..Success";
        auto it = enemy_attacking_economy.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds(all_my_troops, cmd, true, assigned_cmds);
      }
    }
    return true;
}

bool MCRuleActor::GetActSimpleState(vector<int>* state) {
    vector<int> &_state = *state;

    const auto& my_troops = _preload.MyTroops();
    const auto& cnt_under_construction = _preload.CntUnderConstruction();
    //const auto& enemy_troops = _preload.EnemyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();

    _state[STATE_GATHER] = 1;

    if (my_troops[PEASANT].size() < 3 && _preload.Affordable(PEASANT)) {
        _state[STATE_BUILD_PEASANT] = 1;
    }

    if (my_troops[PEASANT].size() >= 3 && my_troops[BARRACK].size() + cnt_under_construction[BARRACK] < 1 && _preload.Affordable(BARRACK)) {
        _state[STATE_BUILD_BARRACK] = 1;
    }

    if (my_troops[BARRACK].size() >= 1 && _preload.Affordable(SWORDMAN)) {
        _state[STATE_BUILD_SWORDMAN] = 1;
    }

    if (my_troops[SWORDMAN].size() >= 1) {
        _state[STATE_SCOUT] = 1;
    }

    if (my_troops[SWORDMAN].size() >= 5) {
        _state[STATE_ATTACK] = 1;
    }
    if (! enemy_troops_in_range.empty() || ! enemy_attacking_economy.empty()) {
        _state[STATE_ATTACK_IN_RANGE] = 1;
        _state[STATE_DEFEND] = 1;
    }
    return true;
}

bool MCRuleActor::GetActHitAndRunState(vector<int>* state) {
    vector<int> &_state = *state;

    const auto& my_troops = _preload.MyTroops();
    const auto& cnt_under_construction = _preload.CntUnderConstruction();
    const auto& enemy_troops = _preload.EnemyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();

    if (my_troops[PEASANT].size() < 3 && _preload.Affordable(PEASANT)) {
        _state[STATE_BUILD_PEASANT] = 1;
    }
    if (my_troops[PEASANT].size() >= 3 && my_troops[BARRACK].size() + cnt_under_construction[BARRACK] < 1 && _preload.Affordable(BARRACK)) {
        _state[STATE_BUILD_BARRACK] = 1;
    }
    if (my_troops[BARRACK].size() >= 1 && _preload.Affordable(CAVALRY)) {
        _state[STATE_BUILD_RANGE_TROOP] = 1;
    }
    int range_troop_size = my_troops[CAVALRY].size();
    if (range_troop_size >= 2) {
        if (enemy_troops[SPEARMAN].empty() && enemy_troops[CAVALRY].empty()
          && enemy_troops[PEASANT].empty()) {
            _state[STATE_ATTACK] = 1;
        } else {
            _state[STATE_HIT_AND_RUN] = 1;
        }
    }
    if (! enemy_troops_in_range.empty() || ! enemy_attacking_economy.empty()) {
        _state[STATE_DEFEND] = 1;
    }
    return true;
}

bool MCRuleActor::ActWithMap(const GameEnv &env, const vector<vector<vector<int>>>& action_map, string *state_string, AssignedCmds *assigned_cmds) {
    assigned_cmds->clear();
    *state_string = "";

    vector<vector<RegionHist>> hist(action_map.size());
    for (size_t i = 0; i < hist.size(); ++i) {
        hist[i].resize(action_map[i].size());
    }

    const int x_size = env.GetMap().GetXSize();
    const int y_size = env.GetMap().GetYSize();
    const int rx = action_map.size();
    const int ry = action_map[0].size();

    // Then loop over all my troops to run.
    const auto& all_my_troops = _preload.AllMyTroops();
    for (const Unit *u : all_my_troops) {
        // Get the bin id.
        const PointF& p = u->GetPointF();
        int x = static_cast<int>(std::round(p.x / x_size * rx));
        int y = static_cast<int>(std::round(p.y / y_size * ry));
        // [REGION_MAX_RANGE_X][REGION_MAX_RANGE_Y][REGION_RANGE_CHANNEL]
        act_per_unit(env, u, &action_map[x][y][0], &hist[x][y], state_string, assigned_cmds);
    }

    return true;
}
