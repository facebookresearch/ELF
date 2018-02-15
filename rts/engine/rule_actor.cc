/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "rule_actor.h"
#include "gamedef.h"

static const float HitAndRunDist1 = 4.0;
static const float HitAndRunDist2 = 6.0;
static const float HitAndRunDist3 = 10.0;

///////////////////////////// RuleActor //////////////////////////////

void Preload::collect_stats(const GameEnv &env, int player_id, const CmdReceiver &receiver) {
    // Clear all data
    //
    _my_troops.clear();
    _enemy_troops.clear();
    _enemy_troops_in_range.clear();
    _all_my_troops.clear();
    _enemy_attacking_economy.clear();
    _economy_being_attacked.clear();
    _cnt_under_construction.clear();
    _num_unit_type = env.GetGameDef().GetNumUnitType();

    // Initialize to a given size.
    _my_troops.resize(_num_unit_type);
    _enemy_troops.resize(_num_unit_type);
    _cnt_under_construction.resize(_num_unit_type, 0);
    _prices.resize(_num_unit_type, 0);
    _result = NOT_READY;

    _player_id = player_id;

    // Collect ...
    const Units& units = env.GetUnits();
    //const RTSMap& m = env.GetMap();
    const Player& player = env.GetPlayer(_player_id);

    // cout << "Looping over units" << endl << flush;

    // Get the information of all other troops.
    for (auto it = units.begin(); it != units.end(); ++it) {
        const Unit *u = it->second.get();
        if (u == nullptr) cout << "Unit cannot be nullptr" << endl << flush;
        // cout << "unit: " << u->GetProperty().PrintInfo() << endl << flush;

        auto &troops = (u->GetPlayerId() == _player_id ? _my_troops : _enemy_troops);
        troops[u->GetUnitType()].push_back(u);

        if (u->GetPlayerId() == _player_id) {
            if (InCmd(receiver, *u, BUILD)) {
                const CmdDurative *curr_cmd = receiver.GetUnitDurativeCmd(u->GetId());
                if (curr_cmd == nullptr) cout << "Cmd cannot be null! id = " << u->GetId() << endl << flush;
                const CmdBuild *curr_cmd_build = dynamic_cast<const CmdBuild *>(curr_cmd);
                if (curr_cmd_build == nullptr) cout << "Current cmd cannot be converted to CmdBuild!" << endl << flush;
                UnitType ut = curr_cmd_build->build_type();
                // if ((int)ut < 0 || (int)ut >= (int)NUM_UNITTYPE) cout << "buidl unit_type is invalid! " << (int)ut << endl << flush;
                _cnt_under_construction[ut] ++;
            }
            _all_my_troops.push_back(u);

            // Check damage from
            UnitType unit_type = u->GetUnitType();
            if (unit_type == WORKER || unit_type == BASE) {
                UnitId damage_from = u->GetProperty().GetLastDamageFrom();
                if (damage_from != INVALID) {
                    const Unit *source = env.GetUnit(damage_from);
                    if (source != nullptr) {
                        _enemy_attacking_economy.push_back(source);
                        _economy_being_attacked.push_back(u);
                    }
                }
            }
        } else {
            if (player.FilterWithFOW(*u)) {
                // Attack if we have troops.
                if (u->GetUnitType() != RESOURCE) {
                    _enemy_troops_in_range.push_back(u);
                }
            }
        }
    }
    make_unique(&_enemy_attacking_economy);
    make_unique(&_economy_being_attacked);
}

void Preload::GatherInfo(const GameEnv& env, int player_id, const CmdReceiver &receiver) {
    // cout << "GatherInfo(): player_id: " << player_id << endl;
    assert(player_id >= 0 && player_id < env.GetNumOfPlayers());
    collect_stats(env, player_id, receiver);
    const Player& player = env.GetPlayer(_player_id);
    _resource = player.GetResource();

    if (!env.GetGameDef().HasBase()) return;

    if (_my_troops[BASE].empty() || _enemy_troops[BASE].empty()) {
        _result = NO_BASE;
        // cout << "_result = NO_BASE" << endl;
        return;
    }

    // cout << "Base not empty" << endl << flush;
    _base = _my_troops[BASE][0];
    _base_id = _base->GetId();
    _base_loc = _my_troops[BASE][0]->GetPointF();
    _opponent_base_id = _enemy_troops[BASE][0]->GetId();

    for (int i = 0; i < _num_unit_type; ++i) {
        _prices[i] = env.GetGameDef().unit((UnitType)i).GetUnitCost();
    }

    _enemy_at_resource = nullptr;
    _enemy_at_base = nullptr;

    // cout << "Check whether resource is empty .." << endl << flush;

    // If there is no resource, just attack opponent's base.
    // Everyone, including workers.
    if (_my_troops[RESOURCE].empty()) {
        _result = NO_RESOURCE;
        // cout << "_result = NO_RESOURCE" << endl;
        return;
    }

    // cout << "Resource info.." << endl << flush;
    _resource_id = _my_troops[RESOURCE][0]->GetId();
    _resource_loc = _my_troops[RESOURCE][0]->GetPointF();
    _result = OK;
}

const Unit *Preload::EnemyAtResource() {
    if (_enemy_at_resource == nullptr) {
      float closest = 6.0;
      _enemy_at_resource = RuleActor::closest_dist(_enemy_troops_in_range, _resource_loc, &closest);
    }
    return _enemy_at_resource;
}

const Unit *Preload::EnemyAtBase() {
    if (_enemy_at_base == nullptr) {
      float closest = 4.0;
      _enemy_at_base = RuleActor::closest_dist(_enemy_troops_in_range, _base_loc, &closest);
    }
    return _enemy_at_base;
}

/////////////////////// RuleActor /////////////////////
bool RuleActor::store_cmd(const Unit *u, CmdBPtr&& cmd, AssignedCmds *m) const {
    if (cmd.get() == nullptr) return false;
    // Check if the same cmd has been issued (in particular ATTACK, since ATTACK has cooldown).
    const CmdDurative *curr_cmd = GetCurrCmd(*_receiver, *u);
    // cout << "id: " << u->GetId() << " " << u->GetUnitType() << " Loc: (" << u->GetPointF() << ") cmd_to_issue: "
    //      << cmd.PrintInfo() << " cmdstate: " << state.cmd << " last_cmd:" << last_cmd << endl;
    //
    if (curr_cmd != nullptr) {
        if (curr_cmd->type() == ATTACK && cmd->type() == ATTACK) {
            const CmdAttack *curr_cmd_att = dynamic_cast<const CmdAttack *>(curr_cmd);
            const CmdAttack *cmd_att = dynamic_cast<const CmdAttack *>(cmd.get());
            if (curr_cmd_att->target() == cmd_att->target()) return false;
        }
    }

    (*m)[u->GetId()] = std::move(cmd);
    return true;
}

void RuleActor::batch_store_cmds(const vector<const Unit *> &subset,
        const CmdBPtr& cmd, bool preemptive, AssignedCmds *m) const {
    for (const Unit *u : subset) {
        const CmdDurative *curr_cmd = GetCurrCmd(*_receiver, *u);
        if (curr_cmd == nullptr || (preemptive && curr_cmd->type() != cmd->type()) ) {
            store_cmd(u, cmd->clone(), m);
        }
    }
}

bool RuleActor::hit_and_run(const GameEnv &env, const Unit *u, const vector<const Unit*> targets,
        AssignedCmds *assigned_cmds) {
    // cout << "Check u " << hex << (void *)u << dec << endl << flush;

    float closest = std::numeric_limits<float>::max();
    const Unit *closest_target = closest_dist(targets, u->GetPointF(), &closest);
    if (closest_target != nullptr) {
        UnitId opponent_target_id = closest_target->GetId();
        if (closest > HitAndRunDist2) {
            store_cmd(u, _A(opponent_target_id), assigned_cmds);
        }
        if (closest < HitAndRunDist1) {
            // try to hit and run
            PointF res_p;
            const PointF &mypf = u->GetPointF();
            if (env.FindClosestPlaceWithDistance(mypf, HitAndRunDist3, targets, &res_p)) {
                store_cmd(u, _M(res_p), assigned_cmds);
            } else {
                //fail, just attack back
                store_cmd(u, _A(opponent_target_id), assigned_cmds);
            }
        }
    }
    return true;
}

bool RuleActor::GatherInfo(const GameEnv &env, string *state_string, AssignedCmds *assigned_cmds) {
    _preload.GatherInfo(env, _player_id, *_receiver);

    auto res = _preload.GetResult();
    if (res == Preload::NO_BASE) return false;
    if (res == Preload::NO_RESOURCE) {
        // cout << "Check whether resource is empty .." << endl << flush;
        // If there is no resource, just attack opponent's base.
        // Everyone, including workers.
        *state_string = "Resource depleted. All-in attack.";
        batch_store_cmds(_preload.AllMyTroops(), _preload.GetAttackEnemyBaseCmd(), true, assigned_cmds);
        return false;
    }
    return true;
}

bool RuleActor::act_per_unit(const GameEnv &env, const Unit *u, const int *state, RegionHist *region_hist, string *state_string, AssignedCmds *assigned_cmds) {
    UnitType ut = u->GetUnitType();
    const CmdDurative *curr_cmd = _receiver->GetUnitDurativeCmd(u->GetId());
    bool idle = (curr_cmd == nullptr);
    CmdType cmdtype = idle ? INVALID_CMD : curr_cmd->type();

    if (ut == BASE && state[STATE_BUILD_WORKER] && idle) {
        if (_preload.BuildIfAffordable(WORKER)) {
            *state_string = "Build worker..Success";
            store_cmd(u, _B_CURR_LOC(WORKER), assigned_cmds);
        }
    }

    // Ask workers to gather.
    if (ut == WORKER) {
        // Gather!
        if (idle) store_cmd(u, _preload.GetGatherCmd(), assigned_cmds);

        // If resource permit, build barracks.
        if (cmdtype == GATHER && state[STATE_BUILD_BARRACK] && ! region_hist->has_built_barracks) {
            if (_preload.Affordable(BARRACK)) {
                *state_string = "Build barracks..Success";
                CmdBPtr cmd = _preload.GetBuildBarracksCmd(env);
                if (cmd != nullptr) {
                    store_cmd(u, std::move(cmd), assigned_cmds);
                    region_hist->has_built_barracks = true;
                    _preload.Build(BARRACK);
                }
            }
        }
    }

    if (ut == BARRACK && idle) {
        if (state[STATE_BUILD_MELEE_TROOP] && ! region_hist->has_built_melee) {
            if (_preload.BuildIfAffordable(TRUCK)) {
                *state_string = "Build Melee Troop..Success";
                store_cmd(u, _B_CURR_LOC(TRUCK), assigned_cmds);
                region_hist->has_built_melee = true;
            }
        }

        if (state[STATE_BUILD_RANGE_TROOP] && ! region_hist->has_built_range) {
            if (_preload.BuildIfAffordable(TANK)) {
                *state_string = "Build Range Troop..Success";
                store_cmd(u, _B_CURR_LOC(TANK), assigned_cmds);
                region_hist->has_built_range = true;
            }
        }
    }

    if (state[STATE_ATTACK] && (ut == TRUCK || ut == TANK)) {
        if (idle) store_cmd(u, _preload.GetAttackEnemyBaseCmd(), assigned_cmds);
    }

    if (state[STATE_HIT_AND_RUN]) {
        // cout << "Enter hit and run procedure" << endl << flush;
        auto enemy_troops = _preload.EnemyTroops();
        *state_string = "Hit and run";
        if (ut == TANK) {
            // cout << "Enemy only have worker" << endl << flush;
            if (enemy_troops[TRUCK].empty() && enemy_troops[TANK].empty() && ! enemy_troops[WORKER].empty()) {
                hit_and_run(env, u, enemy_troops[WORKER], assigned_cmds);
            }

            if (! enemy_troops[TRUCK].empty()) {
                hit_and_run(env, u, enemy_troops[TRUCK], assigned_cmds);
            }
        }
        if (ut == TANK || ut == TRUCK) {
            if (! enemy_troops[TANK].empty() && idle) {
                store_cmd(u, _A(enemy_troops[TANK][0]->GetId()), assigned_cmds);
            }
        }
    }

    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();

    if ((ut == TANK || ut == TRUCK)
            && idle && state[STATE_ATTACK_IN_RANGE] && ! enemy_troops_in_range.empty()) {
        *state_string = "Attack enemy in range..Success";
        auto cmd = _A(enemy_troops_in_range[0]->GetId());
        store_cmd(u, std::move(cmd), assigned_cmds);
    }

    if (state[STATE_DEFEND]) {
      // Group Retaliation. All troops attack.
      *state_string = "Defend enemy attack..NOOP";
      const Unit *enemy_at_resource = _preload.EnemyAtResource();
      if (enemy_at_resource != nullptr) {
          *state_string = "Defend enemy attack..Success";
          store_cmd(u, _A(enemy_at_resource->GetId()), assigned_cmds);
      }

      const Unit *enemy_at_base = _preload.EnemyAtBase();
      if (enemy_at_base != nullptr) {
          *state_string = "Defend enemy attack..Success";
          store_cmd(u, _A(enemy_at_base->GetId()), assigned_cmds);
      }
      if (! enemy_attacking_economy.empty()) {
          *state_string = "Defend enemy attack..Success";
          auto it = enemy_attacking_economy.begin();
          store_cmd(u, _A((*it)->GetId()), assigned_cmds);
      }
    }
    return true;
}
