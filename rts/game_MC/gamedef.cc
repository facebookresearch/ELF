/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "engine/gamedef.h"
#include "engine/game_env.h"
#include "engine/rule_actor.h"

#include "engine/cmd.gen.h"
#include "engine/cmd_specific.gen.h"
#include "cmd_specific.gen.h"
#include "engine/ai.h"
#include "engine/lua/lua_interface.h"
#include "engine/lua/cpp_interface.h"
#include "lua/lua_interface.h"
#include "lua/cpp_interface.h"
#include "rule_ai.h"


int GameDef::GetNumUnitType() {
    return NUM_MINIRTS_UNITTYPE;
}

int GameDef::GetNumAction() {
    return NUM_AISTATE;
}

bool GameDef::IsUnitTypeBuilding(UnitType t) const{
    return (t == TOWN_HALL) || (t == RESOURCE) || (t == BARRACK) || (t == BLACKSMITH) || (t == STABLE) || (t == WORKSHOP);;
}

bool GameDef::HasBase() const{ return true; }

bool GameDef::CheckAddUnit(RTSMap *_map, UnitType unit_type, const PointF& p) const{
    const UnitTemplate& unit_def = _units[unit_type];
    return _map->CanPass(p, INVALID, false, unit_def);
}

void GameDef::GlobalInit() {
    reg_engine();
    reg_engine_specific();
    reg_minirts_specific();
    reg_engine_cmd_lua();
    reg_engine_lua_interfaces();
    reg_engine_cpp_interfaces();
    reg_lua_interfaces();
    reg_cpp_interfaces();

    // InitAI.
    AIFactory<AI>::RegisterAI("simple", [](const std::string &spec) {
        (void)spec;
        AIOptions ai_options;
        return new SimpleAI(ai_options);
    });

    AIFactory<AI>::RegisterAI("hit_and_run", [](const std::string &spec) {
        (void)spec;
        AIOptions ai_options;
        return new HitAndRunAI(ai_options);
    });
}

void GameDef::Init() {
    _units.assign(GetNumUnitType(), UnitTemplate());

    _units[RESOURCE] = RTSUnitFactory::InitResource();
    _units[PEASANT] = RTSUnitFactory::InitPeasant();
    _units[SWORDMAN] = RTSUnitFactory::InitSwordman();
    _units[SPEARMAN] = RTSUnitFactory::InitSpearman();
    _units[CAVALRY] = RTSUnitFactory::InitCavalry();
    _units[ARCHER] = RTSUnitFactory::InitArcher();
    _units[DRAGON] = RTSUnitFactory::InitDragon();
    _units[CATAPULT] = RTSUnitFactory::InitCatapult();
    _units[BARRACK] = RTSUnitFactory::InitBarrack();
    _units[BLACKSMITH] = RTSUnitFactory::InitBlacksmith();
    _units[STABLE] = RTSUnitFactory::InitStables();
    _units[WORKSHOP] = RTSUnitFactory::InitWorkshop();
    _units[GUARD_TOWER] = RTSUnitFactory::InitGuardTower();
    _units[TOWN_HALL] = RTSUnitFactory::InitTownHall();
}

vector<pair<CmdBPtr, int> > GameDef::GetInitCmds(const RTSGameOptions& options) const{
      vector<pair<CmdBPtr, int> > init_cmds;
      init_cmds.push_back(make_pair(CmdBPtr(new CmdGenerateMap(INVALID, options.seed)), 1));
      //init_cmds.push_back(make_pair(CmdBPtr(new CmdGameStartSpecific(INVALID)), 2));
      init_cmds.push_back(make_pair(CmdBPtr(new CmdGenerateUnit(INVALID, options.seed)), 2));
      return init_cmds;
}

PlayerId GameDef::CheckWinner(const GameEnv& env, bool /*exceeds_max_tick*/) const {
    return env.CheckBase(TOWN_HALL);
}

void GameDef::CmdOnDeadUnitImpl(GameEnv* env, CmdReceiver* receiver, UnitId /*_id*/, UnitId _target) const{
    Unit *target = env->GetUnit(_target);
    if (target == nullptr) return; receiver->SendCmd(CmdIPtr(new CmdRemove(_target))); }
