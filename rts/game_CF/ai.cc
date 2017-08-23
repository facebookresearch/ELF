/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "ai.h"
#include "engine/game_env.h"
#include "engine/unit.h"

void AIBase::save_structured_state(const GameEnv &env, Data *data) const {
    GameState *game = &data->newest();
    game->tick = _receiver->GetTick();
    game->winner = env.GetWinnerId();
    game->terminal = env.GetTermination() ? 1 : 0;
    game->game_counter = env.GetGameCounter();
    game->player_id = _player_id;

    const int n_type = env.GetGameDef().GetNumUnitType();
    const int n_additional = 3;
    const int resource_grid = 1;
    const int res_pt = NUM_RES_SLOT;
    const int total_channel = n_type + n_additional + res_pt;

    const auto &m = env.GetMap();

    // [Channel, width, height]
    const int sz = total_channel * m.GetXSize() * m.GetYSize();
    game->s.resize(sz);
    std::fill(game->s.begin(), game->s.end(), 0.0);

    int players = env.GetNumOfPlayers();
    game->res.resize(players * res_pt);
    std::fill(game->res.begin(), game->res.end(), 0.0);

#define _OFFSET(_c, _x, _y) (((_c) * m.GetYSize() + (_y)) * m.GetXSize() + (_x))
#define _F(_c, _x, _y) game->s[_OFFSET(_c, _x, _y)]

    // Extra data.
    game->ai_start_tick = 0;
    auto unit_iter = env.GetUnitIterator(_player_id);
    std::vector<int> quantized_r(players, 0);

    while (! unit_iter.end()) {
        const Unit &u = *unit_iter;
        int x = int(u.GetPointF().x);
        int y = int(u.GetPointF().y);
        float hp_level = u.GetProperty()._hp / (u.GetProperty()._max_hp + 1e-6);
        UnitType t = u.GetUnitType();

        _F(t, x, y) = 1.0;
        _F(n_type, x, y) = u.GetPlayerId() + 1;
        _F(n_type + 1, x, y) = hp_level;
        _F(n_type + 2, x, y) = u.HasFlag();
        if (u.HasFlag()) game->flag_x = x;
        ++ unit_iter;
    }

    for (int i = 0; i < players; ++i) {
        if (_player_id != INVALID && _player_id != i) continue;
        const auto &player = env.GetPlayer(i);
        quantized_r[i] = min(int(player.GetResource() / resource_grid), res_pt - 1);
        game->res[i * res_pt + quantized_r[i]] = 1.0;
    }

    if (_player_id != INVALID) {
        const int c = _OFFSET(n_type + n_additional + quantized_r[_player_id], 0, 0);
        std::fill(game->s.begin() + c, game->s.begin() + c + m.GetXSize() * m.GetYSize(), 1.0);
    }

    game->last_r = 0.0;
    game->r0 = quantized_r[0];
    game->r1 = quantized_r[1];
    int winner = env.GetWinnerId();

    if (winner != INVALID) {
        if (winner == _player_id) game->last_r = 1.0;
        else game->last_r = -1.0;
    }
}

bool FlagTrainedAI::on_act(const GameEnv &env) {
    _state.resize(NUM_FLAGSTATE);
    std::fill (_state.begin(), _state.end(), 0);

    // if (_receiver == nullptr) return false;

    if (! this->need_structured_state(_receiver->GetTick())) {
        // We just use the backup AI.
        // cout << "Use the backup AI, tick = " << _receiver->GetTick() << endl;
        SendComment("BackupAI");
        return _backup_ai->Act(env);
    }
    int h = 0;
    const GameState& gs = _ai_comm->info().data.newest();
    h = gs.a;
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
