/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "../engine/gamedef.h"
#include "../engine/game_env.h"
#include "../engine/rule_actor.h"

#include "../engine/cmd.gen.h"
#include "../engine/cmd_specific.gen.h"
#include "cmd_specific.gen.h"

int GameDef::GetNumUnitType() {
    return NUM_TD_UNITTYPE;
}

//TODO:: fix
int GameDef::GetNumAction() {
    return 400;
}

bool GameDef::IsUnitTypeBuilding(UnitType t) const{
    return t == TOWER_BASE;
}

bool GameDef::HasBase() const{return false; }

bool GameDef::CheckAddUnit(RTSMap *_map, UnitType type, const PointF& p) const{
    if (type == TOWER) return _map->CanBuildTower(p, INVALID);
    return _map->CanPass(p, INVALID);
}

void GameDef::Init() {
    _units.assign(GetNumUnitType(), UnitTemplate());
    _units[TOWER_BASE] = _C(0, 100, 0, 0.0, 9999, 1, 9999, {0, 0, 0, 0}, vector<CmdType>{ATTACK, BUILD_TOWER, TOWER_DEFENSE_WAVE_START});
    _units[TOWER] = _C(50, 100, 0, 0.0, 50, 3, 5, {0, 15, 0, 0}, vector<CmdType>{ATTACK}, ATTR_INVULNERABLE);
    _units[TOWER_ATTACKER] = _C(0, 50, 0, 0.1, 10, 1, 5, {0, 100, 0, 0}, vector<CmdType>{MOVE, ATTACK});
    reg_engine();
    reg_engine_specific();
    reg_td_specific();
}

vector<pair<CmdBPtr, int> > GameDef::GetInitCmds(const RTSGameOptions&) const{
      vector<pair<CmdBPtr, int> > init_cmds;
      init_cmds.push_back(make_pair(CmdBPtr(new CmdTowerDefenseGameStart(INVALID)), 1));
      return init_cmds;
}

PlayerId GameDef::CheckWinner(const GameEnv& env, bool exceeds_max_tick) const{
    if (exceeds_max_tick) return 0;
    return env.CheckBase(TOWER_BASE);
}

void GameDef::CmdOnDeadUnitImpl(GameEnv* env, CmdReceiver* receiver, UnitId _id, UnitId _target) const{
    Unit *target = env->GetUnit(_target);
    if (target == nullptr) return;
    Unit *u = env->GetUnit(_id);
    if (u->GetProperty()._attr == ATTR_INVULNERABLE) {
        receiver->SendCmd(CmdIPtr(new CmdChangePlayerResource(INVALID, u->GetPlayerId(), 10)));
    }
    receiver->SendCmd(CmdIPtr(new CmdRemove(_target)));
}

/*
bool GameDef::ActByStateFunc(RuleActor, const GameEnv&, const vector<int>& , string*, AssignedCmds*) const {
    return false;
}
*/
