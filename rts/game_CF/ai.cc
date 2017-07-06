/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "ai.h"

bool FlagTrainedAI::on_act(const GameEnv &env) {
    _state.resize(NUM_FLAGSTATE);
    std::fill (_state.begin(), _state.end(), 0);

    // if (_receiver == nullptr) return false;

    if (! NeedStructuredState(_receiver->GetTick())) {
        // We just use the backup AI.
        // cout << "Use the backup AI, tick = " << _receiver->GetTick() << endl;
        send_comment("BackupAI");
        return _backup_ai->Act(env);
    }
    // Get the current action from the queue.
    int h = 0;
    const Reply& reply = _ai_comm->newest().reply;
    if (reply.global_action >= 0) {
        // action
        h = reply.global_action;
    }
    _state[h] = 1;
    return gather_decide(env, [&](const GameEnv &e, string*, AssignedCmds *assigned_cmds) {
        return _cf_rule_actor.FlagActByState(e, _state, assigned_cmds);
    });
}

bool FlagSimpleAI::on_act(const GameEnv &env) {
    _state.resize(NUM_FLAGSTATE, 0);
    std::fill (_state.begin(), _state.end(), 0);
    return gather_decide(env, [&](const GameEnv &e, string*, AssignedCmds *assigned_cmds) {
        _cf_rule_actor.GetFlagActSimpleState(e, &_state);
        return _cf_rule_actor.FlagActByState(e, _state, assigned_cmds);
    });
}
