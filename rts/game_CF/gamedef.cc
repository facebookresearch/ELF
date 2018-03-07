/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "engine/gamedef.h"
#include "engine/game.h"
#include "engine/game_env.h"
#include "engine/rule_actor.h"
#include "engine/cmd.gen.h"
#include "engine/cmd_specific.gen.h"

#include "cmd_specific.gen.h"
#include "ai.h"

int GameDef::GetNumUnitType() {
    return NUM_FLAG_UNITTYPE;
}

int GameDef::GetNumAction() {
    return NUM_FLAGSTATE;
}

bool GameDef::IsUnitTypeBuilding(UnitType t) const{
    return t == FLAG_TOWN_HALL;
}

bool GameDef::HasBase() const {return false; }

bool GameDef::CheckAddUnit(RTSMap *_map, UnitType, const PointF& p) const{
    return _map->CanPass(p, INVALID);
}

void GameDef::GlobalInit() {
    reg_engine();
    reg_engine_specific();
    reg_cf_specific();

    // InitAI.
    AI::RegisterAI("flag_simple", [](const std::string &spec) {
        AIOptions ai_options;
        ai_options.fs = std::stoi(spec);
        return new FlagSimpleAI(ai_options, nullptr);
    });
}

void GameDef::Init() {
    _units.assign(GetNumUnitType(), UnitTemplate());
    _units[FLAG_TOWN_HALL] = _C(0, 1, 0, 0.0, 0, 0, 5, {0, 0, 0, 1}, vector<CmdType>{BUILD}, ATTR_INVULNERABLE);
    _units[FLAG] = _C(0, 1, 0, 0, 0, 0, 0, vector<int>{0, 0, 0, 0}, vector<CmdType>{}, ATTR_INVULNERABLE);
    _units[FLAG_ATHLETE] = _C(0, 100, 0, 0.1, 10, 1, 3, vector<int>{0, 15, 0, 0}, vector<CmdType>{MOVE, ATTACK, GET_FLAG, ESCORT_FLAG_TO_TOWN_HALL});
}

vector<pair<CmdBPtr, int> > GameDef::GetInitCmds(const RTSGameOptions& options) const{
      vector<pair<CmdBPtr, int> > init_cmds;
      init_cmds.push_back(make_pair(CmdBPtr(new CmdCaptureFlagGameStart(INVALID, 0)), 1));
      init_cmds.push_back(make_pair(CmdBPtr(new CmdFlagSetHandicap(INVALID, options.handicap_level)), 2));
      return init_cmds;
}

PlayerId GameDef::CheckWinner(const GameEnv& env, bool /*exceeds_max_tick*/) const {
    PlayerId _winner_id = INVALID;
    for (PlayerId player_id = 0; player_id < env.GetNumOfPlayers(); player_id ++) {
        const Player& player = env.GetPlayer(player_id);
        int resource = player.GetResource();
        if (resource >= 5) {
            _winner_id = player_id;
            return _winner_id;
        }
    }
    return _winner_id;
}

void GameDef::CmdOnDeadUnitImpl(GameEnv* env, CmdReceiver* receiver, UnitId /*_id*/, UnitId _target) const {
    Unit *target = env->GetUnit(_target);
    if (target == nullptr) return;

    UnitProperty &p_target = target->GetProperty();
    if (p_target._has_flag == 1) {
        const PointF& curr = target->GetPointF();
        receiver->SendCmdWithTick(CmdIPtr(new CmdCreate(INVALID, FLAG, curr, 2)), receiver->GetNextTick());
    }
    receiver->SendCmd(CmdIPtr(new CmdRemove(_target)));
    receiver->SendCmd(CmdIPtr(new CmdReviveAthlete(INVALID, target->GetPlayerId())));
}

/*
bool GameDef::ActByStateFunc(RuleActor rule_actor, const GameEnv& env, const vector<int>& state, string*, AssignedCmds *cmds) const {
    return rule_actor.FlagActByState(env, state, cmds);
}
*/
