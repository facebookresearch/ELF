/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "ai.h"
#include "float.h"
#include "cmd.gen.h"
//#include "minirts2_state.h"
//#include "minirts3_state.h"

bool AI::gather_decide(const GameEnv &env, std::function<bool (const GameEnv&, string *, AssignedCmds *)> func) {
    string state_string;
    AssignedCmds assigned_cmds;

    // cout << "Before gathering info" << endl << flush;
    // cout << "rule_actor() = " << hex << rule_actor() << dec << endl;
    bool gather_ok = rule_actor()->GatherInfo(env, &state_string, &assigned_cmds);

    // cout << "Before running function" << endl << flush;
    bool act_success = gather_ok ? func(env, &state_string, &assigned_cmds) : true;

    // cout << "Send comments" << endl;
    if (! state_string.empty()) SendComment(state_string);

    // cout << "Actual send comments" << endl;
    actual_send_cmds(env, assigned_cmds);
    return act_success;
}

void AI::actual_send_cmds(const GameEnv &env, AssignedCmds &assigned_cmds) {
    // Finally send these commands.
    for (auto it = assigned_cmds.begin(); it != assigned_cmds.end(); ++it) {
        const Unit *u = env.GetUnit(it->first);
        if (u == nullptr) continue;
        if (! env.GetGameDef().unit(u->GetUnitType()).CmdAllowed(it->second->type())) continue;

        it->second->set_id(it->first);
        // Note that after this command, it->second is not usable.
        add_command(std::move(it->second));
    }
}

void AI::SendComment(const string& s) {
    // Finally send these commands.
    if (_receiver != nullptr) {
        auto cmt = "[" + std::to_string(_player_id) + "] " + s;
        _receiver->SendCmd(CmdBPtr(new CmdComment(INVALID, cmt)));
    }
}
