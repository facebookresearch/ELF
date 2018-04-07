/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "cmd.h"
#include "game_env.h"
#include "cmd_receiver.h"

// Derived class. Note that the definition is automatically generated by a python file.
#include "cmd.h"
#include "cmd.gen.h"
#include "cmd_specific.gen.h"

#include "aux_func.h"
#include "engine/lua/cpp_interface.h"

static const int kMoveToRes = 0;
static const int kGathering = 1;
static const int kMoveToBase = 2;

static const int kMoveToDest = 0;
static const int kBuilding = 1;

static constexpr float kGatherDist = 1;
static constexpr float kBuildDist = 1;

static constexpr float kGatherDistSqr = kGatherDist * kGatherDist;
static constexpr float kBuildDistSqr = kBuildDist * kBuildDist;

/*
CMD_DURATIVE(Attack, UnitId, target);
    Durative attack. Will first move to target until in attack range. Then it issues melee attack if melee, and emits a bullet if ranged.
    This will resut in a default counterattack by target.
CMD_DURATIVE(Move, PointF, p);
    Durative move to a point p.
CMD_DURATIVE(Build, UnitType, build_type, PointF, p = PointF(), int, state = 0);
    Move to point p to build a unit of build_type. Also changes resource if necessary.
CMD_DURATIVE(Gather, UnitId, base, UnitId, resource, int, state = 0);
    Move between base and resources to gather resources.
*/

//////////// Durative Action ///////////////////////
bool CmdMove::run(const GameEnv &env, CmdReceiver *receiver) {
    const Unit *u = env.GetUnit(_id);
    if (u == nullptr) return false;
    if (micro_move(_tick, *u, env, _p, receiver) < kDistEps) _done = true;
    return true;
}

// ----- Attack
// Attack cmd.target_id
bool CmdAttack::run(const GameEnv &env, CmdReceiver *receiver) {
    const Unit *u = env.GetUnit(_id);
    if (u == nullptr) return false;

    const Unit *target = env.GetUnit(_target);
    const Player &player = env.GetPlayer(u->GetPlayerId());

    if (target == nullptr || (player.GetPrivilege() == PV_NORMAL && ! player.FilterWithFOW(*target)))  {
        // The goal is destroyed or is out of FOW, back to idle.
        // FilterWithFOW is checked if the agent is not a KNOW_ALL agent.
        // For example, for AI, they could cheat and attack wherever they want.
        // For normal player you cannot attack a Unit outside the FOW.
        //
        _done = true;
        return true;
    }

    const UnitProperty &property = u->GetProperty();
    const float attack_mult = env.GetGameDef().unit(u->GetUnitType()).GetAttackMultiplier(target->GetUnitType());
    const int damage = static_cast<int>(property._att * attack_mult);
    if (damage == 0) {
        _done = true;
        return true;
    }

    const PointF &curr = u->GetPointF();
    const PointF &target_p = target->GetPointF();

    int dist_sqr_to_enemy = PointF::L2Sqr(curr, target_p);
    bool in_attack_range = (dist_sqr_to_enemy <= property._att_r * property._att_r);
    // cout << "[" << _id << "] dist_sqr_to_enemy[" << _last_cmd.target_id << "] = " << dist_sqr_to_enemy << endl;

    // Otherwise attack.
    if (property.CD(CD_ATTACK).Passed(_tick) && in_attack_range) {
        // Melee delivers attack immediately, long-range will deliver attack via bullet.
        if (property._att_r <= 1.0) {
            receiver->SendCmd(CmdIPtr(new CmdMeleeAttack(_id, _target, -damage)));
        } else {
            receiver->SendCmd(CmdIPtr(new CmdEmitBullet(_id, _target, curr, -damage, 0.2)));
        }
        receiver->SendCmd(CmdIPtr(new CmdCDStart(_id, CD_ATTACK)));
    } else if (! in_attack_range) {
        micro_move(_tick, *u, env, target_p, receiver);
    }

    // In both case, continue this action.
    return true;
}

// ---- Gather
bool CmdGather::run(const GameEnv &env, CmdReceiver *receiver) {
    const Unit *u = env.GetUnit(_id);
    if (u == nullptr) return false;

    // Gather resources back and forth.
    const Unit *resource = env.GetUnit(_resource);
    const Unit *base = env.GetUnit(_base);
    if (resource == nullptr || base == nullptr) {
        // Either resource or base is gone. so we stop.
        _done = true;
        return false;
    }
    //const RTSMap &m = env.GetMap();
    //const PointF &curr = u->GetPointF();
    const UnitProperty &property = u->GetProperty();
    const PointF &res_p = resource->GetPointF();
    const PointF &base_p = base->GetPointF();

    //  cout << "[" << tick << "] [" << cmd_state.state << "]" << " CD_MOVE: "
    //       << property.CD(CD_MOVE).PrintInfo(tick) << "  CD_GATHER: "
    //       << property.CD(CD_GATHER).PrintInfo(tick) << endl;
    switch(_state) {
        case kMoveToRes:
            if (micro_move(_tick, *u, env, res_p, receiver) < kGatherDistSqr) {
                // Switch
                receiver->SendCmd(CmdIPtr(new CmdCDStart(_id, CD_GATHER)));
                _state = kGathering;
            }
            break;
        case kGathering:
            // Stay for gathering.
            if (property.CD(CD_GATHER).Passed(_tick)) {
                receiver->SendCmd(CmdIPtr(new CmdHarvest(_id, _resource, -5)));
                _state = kMoveToBase;
            }
            break;
        case kMoveToBase:
            // Get the location of closest base.
            // base_p = m.FindClosestBase(curr, Player::GetPlayerId(u.GetId()));
            if (micro_move(_tick, *u, env, base_p, receiver) < kGatherDistSqr) {
                // Switch
                receiver->SendCmd(CmdIPtr(new CmdChangePlayerResource(_id, u->GetPlayerId(), 5)));
                _state = kMoveToRes;
            }
            break;
    }
    return true;
}

bool CmdBuild::run(const GameEnv &env, CmdReceiver *receiver) {
    const Unit *u = env.GetUnit(_id);
    if (u == nullptr) return false;

    const PointF& curr = u->GetPointF();
    const UnitProperty &p = u->GetProperty();
    //const Player &player = env.GetPlayer(u->GetPlayerId());
    int cost = env.GetGameDef().unit(_build_type).GetUnitCost();

    // When cmd.p = INVALID, build nearby.
    // Otherwise move to nearby location of cmd.p and build at cmd.p
    // cout << "Build cd: " << property.CD(CD_BUILD).PrintInfo(tick) << endl;
    switch(_state) {
        case kMoveToDest:
            // cout << "build_act stage 0 cmd = " << PrintInfo() << endl;
            if (_p.IsInvalid() || PointF::L2Sqr(curr, _p) < kBuildDistSqr) {
                // Note that when we are out of money, the command CmdChangePlayerResource will terminate this command.
                // cout << "Build cost = " << cost << endl;
                receiver->SendCmd(CmdIPtr(new CmdChangePlayerResource(_id, u->GetPlayerId(), -cost)));
                receiver->SendCmd(CmdIPtr(new CmdCDStart(_id, CD_BUILD)));
                _state = kBuilding;
            } else {
                // Move to nearby location.
                PointF nearby_p;
                if (find_nearby_empty_place(env, *u, _p, &nearby_p)) {
                    micro_move(_tick, *u, env, _p, receiver);
                }
            }
            break;
        case kBuilding:
            // Stay for building.
            if (p.CD(CD_BUILD).Passed(_tick)) {
                PointF build_p;
                if (_p.IsInvalid()) {
                    build_p.SetInvalid();
                    find_nearby_empty_place(env, *u, curr, &build_p);
                } else {
                    build_p = _p;
                }
                if (!env.GetMap().CanPass(build_p, INVALID, true, env.GetGameDef().unit(_build_type))) {
                    // cannot build here, money back, end
                    receiver->SendCmd(CmdIPtr(new CmdChangePlayerResource(_id, u->GetPlayerId(), cost)));
                    _done = true;
                    return true;
                }
                if (! build_p.IsInvalid()) {
                    receiver->SendCmd(CmdIPtr(new CmdCreate(_id, _build_type, build_p, u->GetPlayerId(), cost)));
                    _done = true;
                }
            }
            break;
    }
    return true;
}

bool CmdMeleeAttack::run(GameEnv *env, CmdReceiver *receiver) {
    Unit *target = env->GetUnit(_target);
    if (target == nullptr) return true;

    UnitProperty &p_target = target->GetProperty();
    if (p_target.IsDead()) return true;

    const auto &target_tp = env->GetGameDef().unit(target->GetUnitType());

    int changed_hp = _att;
    if (changed_hp < 0) {
        changed_hp += p_target._def;
        if (changed_hp > 0) changed_hp = 0;
    }

    p_target._hp += changed_hp;
    if (p_target.IsDead()) {
        receiver->SendCmd(CmdIPtr(new CmdOnDeadUnit(_id, _target)));
    } else if (changed_hp < 0) {
        p_target._changed_hp = changed_hp;
        p_target._damage_from = _id;
        if (receiver->GetUnitDurativeCmd(_target) == nullptr && target_tp.CmdAllowed(ATTACK)) {
            // Counter attack.
            receiver->SendCmd(CmdDPtr(new CmdAttack(_target, _id)));
        }
    }
    return true;
}

bool CmdChangePlayerResource::run(GameEnv *env, CmdReceiver *receiver) {
    Player &player = env->GetPlayer(_player_id);
    int curr_resource = player.ChangeResource(_delta);
    if (curr_resource < 0) {
        // Change it back and cancel the durative command of _id.
        // cout << "Cancel the build from " << _id << " amount " << _delta << " player_id " << _player_id << endl;
        player.ChangeResource(-_delta);
        receiver->FinishDurativeCmd(_id);
        return false;
    }
    return true;
}

bool CmdIssueInstruction::run(GameEnv *env, CmdReceiver *receiver) {
    auto& player = env->GetPlayer(_player_id);
    player.IssueInstruction(_tick, _instruction);
    env->UnfreezeGame();
    return true;
}

bool CmdFinishInstruction::run(GameEnv *env, CmdReceiver *receiver) {
    auto& player = env->GetPlayer(_player_id);
    player.FinishInstruction(_tick);
    env->FreezeGame();
    return true;
}

bool CmdInterruptInstruction::run(GameEnv *env, CmdReceiver *receiver) {
    auto& player = env->GetPlayer(_player_id);
    player.FinishInstruction(_tick);
    env->FreezeGame();
    return true;
}

bool CmdOnDeadUnit::run(GameEnv *env, CmdReceiver *receiver) {
    env->GetGameDef().CmdOnDeadUnitImpl(env, receiver, _id, _target);
    return true;
}

// For gathering.
bool CmdHarvest::run(GameEnv *env, CmdReceiver *receiver) {
    Unit *target = env->GetUnit(_target);
    if (target == nullptr) return false;

    UnitProperty &p_target = target->GetProperty();
    if (p_target.IsDead()) return false;

    p_target._hp += _delta;
    if (p_target.IsDead()) receiver->SendCmd(CmdIPtr(new CmdRemove(_target)));
    return true;
}
