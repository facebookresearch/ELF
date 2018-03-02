#include "lua_env.h"
#include "cmd_receiver.h"
#include "game_env.h"

#include "cmd.h"
#include "cmd.gen.h"
#include "cmd_specific.gen.h"

#include "aux_func.h"

LuaEnv::LuaEnv() : s_{true} {
    s_["Unit"].SetClass<LuaUnit>(
        "isdead", &LuaUnit::isdead,
        "p", &LuaUnit::p,
        "hp", &LuaUnit::hp, 
        "att", &LuaUnit::att,
        "def", &LuaUnit::def,
        "att_r", &LuaUnit::att_r,
        "player_id", &LuaUnit::player_id,
        "cd_expired", &LuaUnit::CDExpired,
        "can_see", &LuaUnit::CanSee,
        "info", &LuaUnit::info
    );
    s_["Env"].SetClass<LuaEnv>(
        "unit", &LuaEnv::GetUnit, 
        "tick", &LuaEnv::Tick,
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

    s_["PointF"].SetClass<PointF>(
        "isvalid", &PointF::IsValid, 
        "info", &PointF::info);

    s_["global"]["CMD_COMPLETE"] = static_cast<int>(LUA_CMD_COMPLETE);
    s_["global"]["CMD_FAILED"] = static_cast<int>(LUA_CMD_FAILED);
    s_["global"]["CMD_NORMAL"] = static_cast<int>(LUA_CMD_NORMAL);
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

int LuaEnv::Tick() const {
    return receiver_->GetTick();
}

double LuaEnv::DistSqr(const PointF &target_p) {
    return PointF::L2Sqr(unit_.p_ref(), target_p);
}

PointF LuaEnv::FindNearbyEmptyPlace(const LuaUnit& u, const PointF &p) {
    PointF nearby_p;
    find_nearby_empty_place(*env_, u.get(), p, &nearby_p);
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
    // std::cout << "CurrTick: " << receiver_->GetTick() << std::endl;
    return LuaUnit(
        receiver_->GetTick(),
        id, 
        env_->GetPlayer(Player::ExtractPlayerId(id)),
        env_->GetUnit(id)
    );
} 

void reg_engine_cmd_lua() {
    CmdTypeLookup::RegCmdType(CMD_DURATIVE_LUA, "CMD_DURATIVE_LUA");
    
    using CmdLuaAttack = CmdDurativeLuaT<UnitId>;
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdLuaAttack);
    // SERIALIZER_ANCHOR_FUNC(CmdDurative, CmdLuaAttack);

    using CmdLuaGather = CmdDurativeLuaT<UnitId, UnitId>;
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdLuaGather);
    // SERIALIZER_ANCHOR_FUNC(CmdDurative, CmdLuaGather);
    //
    using CmdLuaBuild = CmdDurativeLuaT<int, PointF>;
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdLuaBuild);
    // SERIALIZER_ANCHOR_FUNC(CmdDurative, CmdLuaBuild);
}
