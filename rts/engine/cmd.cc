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
#include "common.h"
#include "gamedef.h"
#include "cmd.gen.h"

SERIALIZER_ANCHOR_INIT(CmdBase);
SERIALIZER_ANCHOR_INIT(CmdImmediate);
SERIALIZER_ANCHOR_INIT(CmdDurative);

std::map<std::string, int> CmdTypeLookup::_name2idx;
std::map<int, std::string> CmdTypeLookup::_idx2name;
std::string CmdTypeLookup::_null;
std::mutex CmdTypeLookup::_mutex;

bool CmdDurative::Run(const GameEnv &env, CmdReceiver *receiver) {
    // If we run this command for the first time, register it in the receiver.
    if (just_started()) receiver->StartDurativeCmd(this);

    // Run the script.
    bool ret = run(env, receiver);

    _tick = receiver->GetNextTick();
    return ret;
}

// ============= Commands ==============
// ----- Move
bool CmdTacticalMove::run(GameEnv *env, CmdReceiver*) {
    Unit *u = env->GetUnit(_id);
    if (u == nullptr) return false;
    RTSMap &m = env->GetMap();

    // Move a unit.
    if (m.MoveUnit(_id, _p)) {
        u->SetPointF(_p);
        u->GetProperty().CD(CD_MOVE).Start(_tick);
        return true;
    }
    else return false;
}

bool CmdCDStart::run(GameEnv *env, CmdReceiver*) {
    Unit *u = env->GetUnit(_id);
    if (u == nullptr) return false;
    u->GetProperty().CD(_cd_type).Start(_tick);
    return true;
}

bool CmdEmitBullet::run(GameEnv *env, CmdReceiver*) {
    if (_id == INVALID) return false;
    Unit *u = env->GetUnit(_id);
    if (u == nullptr) return false;

    // cout << "Bullet: " << micro_cmd.PrintInfo() << endl;
    Bullet b(_p, _id, _att, _speed);
    b.SetTargetUnitId(_target);
    env->AddBullet(b);
    return true;
}

bool CmdCreate::run(GameEnv *env, CmdReceiver*) {
    // Create a unit at a location
    if (! env->AddUnit(_tick, _build_type, _p, _player_id)) {
        // If failed, money back!
        env->GetPlayer(_player_id).ChangeResource(_resource_used);
        return false;
    }
    return true;
}

bool CmdRemove::run(GameEnv *env, CmdReceiver* receiver) {
    if (env->RemoveUnit(_id)) {
        receiver->FinishDurativeCmd(_id);
        return true;
    } else return false;
}

bool CmdLoadMap::run(GameEnv *env, CmdReceiver*) {
    serializer::loader loader(false);
    if (! loader.read_from_file(_s)) return false;
    loader >> env->GetMap();
    return true;
}

bool CmdSaveMap::run(GameEnv *env, CmdReceiver*) {
    serializer::saver saver(false);
    saver << env->GetMap();
    if (! saver.write_to_file(_s)) return false;
    return true;
}

bool CmdRandomSeed::run(GameEnv *env, CmdReceiver*) {
    env->SetSeed(_seed);
    return true;
}

bool CmdComment::run(GameEnv*, CmdReceiver*) {
    // COMMENT command does not do anything, but leave a record.
    return true;
}

