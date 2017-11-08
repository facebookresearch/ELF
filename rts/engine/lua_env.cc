#include "lua_env.h"
#include "cmd_receiver.h"
#include "game_env.h"

#include "cmd.h"
#include "cmd.gen.h"
#include "cmd_specific.gen.h"

#include "aux_func.h"

LuaEnv::LuaEnv() {
    s_["Unit"].SetClass<LuaUnit>(
        "isdead", &LuaUnit::isdead,
        "p", &LuaUnit::p,
        "hp", &LuaUnit::hp, 
        "att", &LuaUnit::att,
        "def", &LuaUnit::def,
        "cd_expired", &LuaUnit::CDExpired,
        "can_see", &LuaUnit::CanSee,
        "info", &LuaUnit::info
    );
    s_["Env"].SetClass<LuaEnv>(
        "unit", &LuaEnv::GetUnit, 
        "self", &LuaEnv::GetSelf,
        "cd_start", &LuaEnv::CDStart, 
        "dist_sqr", &LuaEnv::DistSqr, 
        "find_nearby_empty_place", &LuaEnv::FindNearbyEmptyPlace, 
        "move_towards", &LuaEnv::MoveTowards, 
        "move_towards_target", &LuaEnv::MoveTowardsTarget, 
        "send_cmd_melee_attack", &LuaEnv::SendCmdMeleeAttack, 
        "send_cmd_emit_bullet", &LuaEnv::SendCmdEmitBullet, 
        "send_cmd_harvest_resource", &LuaEnv::HarvestResource, 
        "send_cmd_change_resource", &LuaEnv::ChangeResource, 
        "send_cmd_create", &LuaEnv::SendCmdCreate,
        "unit_cost", &LuaEnv::UnitCost);

    s_["global"]["CMD_COMPLETE"] = static_cast<int>(LUA_CMD_COMPLETE);
    s_["global"]["CMD_FAILED"] = static_cast<int>(LUA_CMD_FAILED);
    s_["global"]["CD_ATTACK"] = static_cast<int>(CD_ATTACK);
    s_["global"]["CD_MOVE"] = static_cast<int>(CD_MOVE);
    s_["global"]["CD_BUILD"] = static_cast<int>(CD_BUILD);
    s_["global"]["CD_GATHER"] = static_cast<int>(CD_GATHER);

    s_.Load("test2.lua");
}

LuaEnv &_get_lua_env(const GameEnv &env, CmdReceiver *receiver) {
    return receiver->GetLuaEnv().Set(env, receiver);
}

void LuaEnv::CDStart(int cd_type) {
    receiver_->SendCmd(CmdIPtr(new CmdCDStart(cmd_->id(), (CDType)cd_type))); 
}

double LuaEnv::DistSqr(const PointF &target_p) {
    return PointF::L2Sqr(unit_.p_ref(), target_p);
}

PointF LuaEnv::FindNearbyEmptyPlace(const PointF &p) {
    PointF nearby_p;
    find_nearby_empty_place(env_->GetMap(), p, &nearby_p);
    return nearby_p;
}

double LuaEnv::MoveTowardsTarget(const PointF &target_p) {
    return micro_move(receiver_->GetTick(), unit_.get(), *env_, target_p, receiver_);
}

double LuaEnv::MoveTowards(const LuaUnit &u) {
    return micro_move(receiver_->GetTick(), unit_.get(), *env_, u.p_ref(), receiver_);
}

void LuaEnv::SendCmdMeleeAttack(UnitId target, int att) {
    receiver_->SendCmd(CmdIPtr(new CmdMeleeAttack(cmd_->id(), target, -att)));
}

void LuaEnv::SendCmdEmitBullet(UnitId target, int att) {
    receiver_->SendCmd(CmdIPtr(new CmdEmitBullet(cmd_->id(), target, unit_.p_ref(), -att, 0.2)));
}

void LuaEnv::SendCmdCreate(int build_type, const PointF &build_p) {
    int cost = UnitCost(build_type);
    receiver_->SendCmd(CmdIPtr(new CmdCreate(cmd_->id(), (UnitType)build_type, build_p, unit_.player_id(), cost)));
}

void LuaEnv::HarvestResource(UnitId res, int delta) {
    receiver_->SendCmd(CmdIPtr(new CmdHarvest(cmd_->id(), res, -delta)));
} 

void LuaEnv::ChangeResource(int delta) {
    receiver_->SendCmd(CmdIPtr(new CmdChangePlayerResource(cmd_->id(), unit_.player_id(), delta)));
}

int LuaEnv::UnitCost(int unit_type) {
    return env_->GetGameDef().unit((UnitType)unit_type).GetUnitCost();
}

LuaUnit LuaEnv::GetUnit(UnitId id) const {
    return LuaUnit(
        id, 
        env_->GetPlayer(Player::ExtractPlayerId(id)),
        env_->GetUnit(id)
    );
} 

