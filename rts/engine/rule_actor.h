/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _RULE_ACTOR_H_
#define _RULE_ACTOR_H_

#include "cmd.h"
#include "cmd_specific.gen.h"
#include "cmd_interface.h"
#include "game_env.h"
#include "lua_env.h"
#include <sstream>
#include <algorithm>

custom_enum(AIState, STATE_START = 0, STATE_BUILD_WORKER, STATE_BUILD_BARRACK,
  STATE_BUILD_MELEE_TROOP, STATE_BUILD_RANGE_TROOP, STATE_ATTACK,
  STATE_ATTACK_IN_RANGE, STATE_HIT_AND_RUN, STATE_DEFEND, NUM_AISTATE);

custom_enum(FlagState, FLAGSTATE_START = 0, FLAGSTATE_GET_FLAG, FLAGSTATE_ATTACK_FLAG,
    FLAGSTATE_ESCORT_FLAG, FLAGSTATE_PROTECT_FLAG, //FLAGSTATE_ATTACK, FLAGSTATE_MOVE,
    NUM_FLAGSTATE);

// Some easy macros
#define _M(...) CmdBPtr(new CmdMove(INVALID, __VA_ARGS__))
// #define _A(target) CmdBPtr(new CmdAttack(INVALID, target))
#define _A(target) CmdBPtr(new CmdDurativeLuaT<UnitId>("attack", std::vector<std::string>{ "target" }, target))
#define _G(...) CmdBPtr(new CmdGather(INVALID, __VA_ARGS__))
#define _B(...) CmdBPtr(new CmdBuild(INVALID, __VA_ARGS__))

// Region commands.
// BUILD_WORKER: for all idle bases in this region, build a worker.
// BUILD_BARRACK: Pick an idle/gathering worker in this region and build a barrack.
// BUILD_MELEE_TROOP: For all barracks in this region, build melee troops.
// BUILD_RANGE_TROOP: For all barracks in this region, build range troops.
// ATTACK:            For all troops (except for workers) in this region, attack the opponent base.
// ATTACK_IN_RANGE:   For all troops (including workers) in this region, attack enemy in range.
custom_enum(AIStateRegion, SR_NOCHANGE = 0, SR_BUILD_WORKER, SR_BUILD_BARRACK, SR_BUILD_MELEE_TROOP, SR_BUILD_RANGE_TROOP, SR_ATTACK, SR_ATTACK_IN_RANGE, NUM_SR);

// Region history.
struct RegionHist {
    bool has_built_barracks;
    bool has_built_melee;
    bool has_built_range;

    RegionHist() {
        has_built_barracks = false;
        has_built_melee = false;
        has_built_range = false;
    }
};

// Class to preload information from game environment for future use.
class Preload {
public:
    enum Result { NOT_READY = -1, OK = 0, NO_BASE, NO_RESOURCE };

private:
    vector<vector<const Unit*> > _my_troops;
    vector<vector<const Unit*> > _enemy_troops;
    vector<const Unit*> _enemy_troops_in_range;
    vector<const Unit*> _all_my_troops;
    vector<const Unit*> _enemy_attacking_economy;
    vector<const Unit*> _economy_being_attacked;
    vector<int> _cnt_under_construction;

    vector<int> _prices;
    int _resource;
    UnitId _base_id, _resource_id, _opponent_base_id;
    PointF _base_loc, _resource_loc;

    const Unit *_base = nullptr;
    PlayerId _player_id = INVALID;
    int _num_unit_type = 0;
    Result _result = NOT_READY;
    const Unit *_enemy_at_resource = nullptr;
    const Unit *_enemy_at_base = nullptr;

    static bool InCmd(const CmdReceiver &receiver, const Unit &u, CmdType cmd) {
        const CmdDurative *c = receiver.GetUnitDurativeCmd(u.GetId());
        return (c != nullptr && c->type() == cmd);
    }

    static void make_unique(vector<const Unit *> *us) {
        set<const Unit *> tmp;
        vector<const Unit *> res;
        for (const Unit *u : *us) {
            if (tmp.find(u) == tmp.end()) {
                tmp.insert(u);
                res.push_back(u);
            }
        }
        *us = res;
    }

    void collect_stats(const GameEnv &env, int player_id, const CmdReceiver &receiver);

public:
    Preload() { }

    void GatherInfo(const GameEnv &env, int player_id, const CmdReceiver &receiver);
    Result GetResult() const { return _result; }

    bool BuildIfAffordable(UnitType ut) {
        if (_resource >= _prices[ut]) {
            _resource -= _prices[ut];
            return true;
        } else return false;
    }
    bool Affordable(UnitType ut) const { return _resource >= _prices[ut]; }
    void Build(UnitType ut) { _resource -= _prices[ut]; }

    CmdBPtr GetGatherCmd() const { return _G(_base_id, _resource_id); }
    CmdBPtr GetAttackEnemyBaseCmd() const { return _A(_opponent_base_id); }
    CmdBPtr GetBuildBarracksCmd(const GameEnv &env) const {
        PointF p;
        if (env.FindEmptyPlaceNearby(_base_loc, 3, &p) && ! p.IsInvalid()) return _B(BARRACKS, p);
        else return CmdBPtr();
    }

    const PointF &ResourceLoc() const { return _resource_loc; }
    const PointF &BaseLoc() const { return _base_loc; }
    const Unit *Base() const { return _base; }
    int Price(UnitType ut) const { return _prices[ut]; }
    int Resource() const { return _resource; }

    const Unit *EnemyAtResource();
    const Unit *EnemyAtBase();

    const vector<vector<const Unit*> > &MyTroops() const { return _my_troops; }
    const vector<vector<const Unit*> > &EnemyTroops() const { return _enemy_troops; }
    const vector<const Unit*> &EnemyTroopsInRange() const { return _enemy_troops_in_range; }
    const vector<const Unit*> &EnemyAttackingEconomy() const { return _enemy_attacking_economy; }
    const vector<const Unit*> &AllMyTroops() const { return _all_my_troops; }
    const vector<int> &CntUnderConstruction() const { return _cnt_under_construction; }
};

// Information of the game used for AI decision.
class RuleActor {
protected:
    const CmdReceiver *_receiver;
    Preload _preload;
    PlayerId _player_id;
    bool hit_and_run(const GameEnv &env, const Unit *u, const vector<const Unit*> targets, AssignedCmds *assigned_cmds);
    bool store_cmd(const Unit *, CmdBPtr &&cmd, AssignedCmds *m) const;
    bool store_cmd_if_different(const Unit *, CmdBPtr &&cmd, AssignedCmds *m) const;
    void batch_store_cmds(const vector<const Unit *> &subset, const CmdBPtr& cmd, bool preemptive, AssignedCmds *m) const;

    bool act_per_unit(const GameEnv &env, const Unit *u, const int *state, RegionHist *region_hist, string *state_string, AssignedCmds *assigned_cmds);

public:
    RuleActor() : _receiver(nullptr), _player_id(INVALID) {
    }

    void SetReceiver(const CmdReceiver *receiver) {
        _receiver = receiver;
    }
    void SetPlayerId(PlayerId id) {
        _player_id = id;
    }
    // Gather information needed for action.
    bool GatherInfo(const GameEnv &env, string *state_string, AssignedCmds *assigned_cmds);

    bool ActByCmd(const GameEnv &env, const vector<CmdInput>& cmd_inputs, string * /*state_string*/, AssignedCmds *assigned_cmds) {
        for (const CmdInput &cmd : cmd_inputs) {
            // std::cout << cmd.info() << std::endl;
            CmdBPtr c = cmd.GetCmd();
            if (c.get() != nullptr) {
                const Unit *u = env.GetUnit(cmd.id);
                if (u != nullptr) {
                    store_cmd(u, std::move(c), assigned_cmds);
                }
            }
        }
        return true;
    }

    static const Unit *closest_dist(const vector<const Unit *>& units, const PointF &p, float *closest) {
        float closest_dist = *closest;
        const Unit *closest_unit = nullptr;
        for (const auto *u : units) {
            float dist_sqr = PointF::L2Sqr(u->GetPointF(), p);
            if (closest_dist > dist_sqr) {
                closest_unit = u;
                closest_dist = dist_sqr;
            }
        }
        *closest = closest_dist;
        return closest_unit;
    }

    static const CmdDurative *GetCurrCmd(const CmdReceiver &receiver, const Unit &u) {
        return receiver.GetUnitDurativeCmd(u.GetId());
    }

    static bool IsIdle(const CmdReceiver &receiver, const Unit &u) {
        return receiver.GetUnitDurativeCmd(u.GetId()) == nullptr;
    }
};

#endif
