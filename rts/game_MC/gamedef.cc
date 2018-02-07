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
    return (t == BASE) || (t == RESOURCE) || (t == BARRACKS);
}

bool GameDef::HasBase() const{ return true; }

bool GameDef::CheckAddUnit(RTSMap *_map, UnitType, const PointF& p) const{
    return _map->CanPass(p, INVALID);
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
    /*
    _units[RESOURCE] = _C(0, 1000, 1000, 0, 0, 0, 0, vector<int>{0, 0, 0, 0}, vector<CmdType>{}, ATTR_INVULNERABLE);
    _units[WORKER] = _C(50, 50, 0, 0.1, 2, 1, 3, vector<int>{0, 10, 40, 40}, vector<CmdType>{MOVE, ATTACK, BUILD, GATHER});
    _units[MELEE_ATTACKER] = _C(100, 100, 1, 0.1, 15, 1, 3, vector<int>{0, 15, 0, 0}, vector<CmdType>{MOVE, ATTACK});
    _units[RANGE_ATTACKER] = _C(100, 50, 0, 0.2, 10, 5, 5, vector<int>{0, 10, 0, 0}, vector<CmdType>{MOVE, ATTACK});
    _units[BARRACKS] = _C(200, 200, 1, 0.0, 0, 0, 5, vector<int>{0, 0, 0, 50}, vector<CmdType>{BUILD});
    _units[BASE] = _C(500, 500, 2, 0.0, 0, 0, 5, {0, 0, 0, 50}, vector<CmdType>{BUILD});
    */

    _units[RESOURCE] = RTSUnitFactory::InitResource();
    _units[WORKER] = RTSUnitFactory::InitWorker();
    _units[MELEE_ATTACKER] = RTSUnitFactory::InitMeleeAttacker();
    _units[RANGE_ATTACKER] = RTSUnitFactory::InitRangeAttacker();
    _units[FLIGHT] = RTSUnitFactory::InitFlight();
    _units[BARRACKS] = RTSUnitFactory::InitBarracks();
    _units[BASE] = RTSUnitFactory::InitBase();

    //cout << _units[RESOURCE]._property._attr << endl;
    //cout << _units[WORKER]._property._attr << endl;
    //cout << _units[MELEE_ATTACKER]._property._attr << endl;
}

vector<pair<CmdBPtr, int> > GameDef::GetInitCmds(const RTSGameOptions& options) const{
      vector<pair<CmdBPtr, int> > init_cmds;
      init_cmds.push_back(make_pair(CmdBPtr(new CmdGenerateMap(INVALID, options.seed)), 1));
      init_cmds.push_back(make_pair(CmdBPtr(new CmdGameStartSpecific(INVALID)), 2));
      //init_cmds.push_back(make_pair(CmdBPtr(new CmdGenerateUnit(INVALID, options.seed)), 2));
      return init_cmds;
}

PlayerId GameDef::CheckWinner(const GameEnv& env, bool /*exceeds_max_tick*/) const {
    return env.CheckBase(BASE);
}

void GameDef::CmdOnDeadUnitImpl(GameEnv* env, CmdReceiver* receiver, UnitId /*_id*/, UnitId _target) const{
    Unit *target = env->GetUnit(_target);
    if (target == nullptr) return;
    receiver->SendCmd(CmdIPtr(new CmdRemove(_target)));
}
