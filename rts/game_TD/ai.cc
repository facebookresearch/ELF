/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "ai.h"
#include "../engine/game_env.h"
#include "../engine/unit.h"

void AIBase::save_structured_state(const GameEnv &env, ExtGame *game) const {
    game->tick = _receiver->GetTick();
    game->winner = env.GetWinnerId();
    game->terminated = env.GetTermination();
    game->game_counter = env.GetGameCounter();
    game->player_id = _player_id;
    const int total_channel = 2;

    const auto &m = env.GetMap();

    // [Channel, width, height]
    const int sz = total_channel * m.GetXSize() * m.GetYSize();
    game->features.resize(sz);
    std::fill(game->features.begin(), game->features.end(), 0.0);

#define _OFFSET(_c, _x, _y) (((_c) * m.GetYSize() + (_y)) * m.GetXSize() + (_x))
#define _F(_c, _x, _y) game->features[_OFFSET(_c, _x, _y)]

    for (int i = 0; i < m.GetXSize(); i++) {
        for (int j = 0; j < m.GetYSize(); j++) {
            _F(0, i, j) = m(m.GetLoc(i, j)).type;
        }
    }
    auto unit_iter = env.GetUnitIterator(_player_id);
    while (! unit_iter.end()) {
        const Unit &u = *unit_iter;
        int x = int(u.GetPointF().x);
        int y = int(u.GetPointF().y);
        _F(1, x, y) = 1.0;
        UnitType t = u.GetUnitType();
        if ((u.GetPlayerId() == 0) && t == TOWER_BASE) {
            game->base_hp_level = u.GetProperty()._hp / (u.GetProperty()._max_hp + 1e-6);
        }
        ++ unit_iter;
    }
    game->last_reward = 0.0;
    int winner = env.GetWinnerId();

    if (winner != INVALID) {
        if (winner == _player_id) game->last_reward = 1.0;
        else game->last_reward = -1.0;
    }
}

bool TDTrainedAI::on_act(const GameEnv &env) {
    // Get the current action from the queue.
    int h = 0;
    const Reply& reply = _ai_comm->newest().reply;
    if (reply.global_action >= 0) {
        // action
        h = reply.global_action;
    }
    return gather_decide(env, [&](const GameEnv &e, string*, AssignedCmds *assigned_cmds) {
        return _td_rule_actor.TowerDefenseActByState(e, h, assigned_cmds);
    });
}

bool TDSimpleAI::on_act(const GameEnv &env) {
    return gather_decide(env, [&](const GameEnv &e, string*, AssignedCmds *assigned_cmds) {
        return _td_rule_actor.ActTowerDefenseSimple(e, assigned_cmds);
    });
}

bool TDBuiltInAI::on_act(const GameEnv &env) {
    return gather_decide(env, [&](const GameEnv &e, string*, AssignedCmds *assigned_cmds) {
        return _td_rule_actor.ActTowerDefenseBuiltIn(e, assigned_cmds);
    });
}
