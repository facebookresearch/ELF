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

void AIBase::save_structured_state(const GameEnv &env, Data *data) const {
    GameState *game = &data->newest();
    game->tick = _receiver->GetTick();
    game->winner = env.GetWinnerId();
    game->terminal = env.GetTermination() ? 1 : 0;
    game->player_id = _player_id;

    const int n_type = env.GetGameDef().GetNumUnitType();
    const int n_additional = 2;
    const int resource_grid = 50;
    const int res_pt = NUM_RES_SLOT;
    const int total_channel = n_type + n_additional + res_pt;

    const auto &m = env.GetMap();

    // [Channel, width, height]
    const int sz = total_channel * m.GetXSize() * m.GetYSize();
    game->s.resize(sz);
    std::fill(game->s.begin(), game->s.end(), 0.0);

    game->res.resize(env.GetNumOfPlayers() * res_pt);
    std::fill(game->res.begin(), game->res.end(), 0.0);

#define _OFFSET(_c, _x, _y) (((_c) * m.GetYSize() + (_y)) * m.GetXSize() + (_x))
#define _F(_c, _x, _y) game->s[_OFFSET(_c, _x, _y)]

    // Extra data.
    game->ai_start_tick = 0;

    PlayerId visibility_check = _respect_fow ? _player_id : INVALID;

    auto unit_iter = env.GetUnitIterator(visibility_check);
    float total_hp_ratio = 0.0;

    int myworker = 0;
    int mytroop = 0;
    int mybarrack = 0;

    std::vector<int> quantized_r(env.GetNumOfPlayers(), 0);

    while (! unit_iter.end()) {
        const Unit &u = *unit_iter;
        int x = int(u.GetPointF().x);
        int y = int(u.GetPointF().y);
        float hp_level = u.GetProperty()._hp / (u.GetProperty()._max_hp + 1e-6);
        UnitType t = u.GetUnitType();

        _F(t, x, y) = 1.0;
        _F(n_type, x, y) = u.GetPlayerId() + 1;
        _F(n_type + 1, x, y) = hp_level;

        total_hp_ratio += hp_level;

        if (u.GetPlayerId() == _player_id) {
            if (t == WORKER) myworker += 1;
            else if (t == MELEE_ATTACKER || t == RANGE_ATTACKER) mytroop += 1;
            else if (t == BARRACKS) mybarrack += 1;
       }

        ++ unit_iter;
    }

    myworker = min(myworker, 3);
    mytroop = min(mytroop, 5);
    mybarrack = min(mybarrack, 1);

    for (int i = 0; i < env.GetNumOfPlayers(); ++i) {
        // Omit player signal from other player's perspective.
        if (visibility_check != INVALID && visibility_check != i) continue;
        const auto &player = env.GetPlayer(i);
        quantized_r[i] = min(int(player.GetResource() / resource_grid), res_pt - 1);
        game->res[i * res_pt + quantized_r[i]] = 1.0;
    }

    if (_player_id != INVALID) {
        // Add resource layer for the current player.
        const int c = _OFFSET(n_type + n_additional + quantized_r[_player_id], 0, 0);
        std::fill(game->s.begin() + c, game->s.begin() + c + m.GetXSize() * m.GetYSize(), 1.0);
    }

    game->last_r = 0.0;
    int winner = env.GetWinnerId();

    if (winner != INVALID) {
        if (winner == _player_id) game->last_r = 1.0;
        else game->last_r = -1.0;
    }
}


bool TrainedAI2::on_act(const GameEnv &env) {
    _state.resize(NUM_AISTATE);
    std::fill (_state.begin(), _state.end(), 0);

    // if (_receiver == nullptr) return false;
    Tick t = _receiver->GetTick();

    if (! this->need_structured_state(t)) {
        // We just use the backup AI.
        // cout << "Use the backup AI, tick = " << _receiver->GetTick() << endl;
        SendComment("BackupAI");
        return _backup_ai->Act(env);
    }
    // Get the current action from the queue.
    int num_action = NUM_AISTATE;
    int h = num_action - 1;
    const GameState& gs = _ai_comm->info().data.newest();
    // uint64_t hash_code;

    switch(gs.action_type) {
        case ACTION_GLOBAL:
            // action
            h = gs.a;
            _state[h] = 1;

            // hash_code = serializer::hash_obj(*_ai_comm->GetData());
            // cout << "[" << t << "]: hash = " << hex << hash_code << dec << ", h = " << h << endl;
            return gather_decide(env, [&](const GameEnv &e, string *s, AssignedCmds *assigned_cmds) {
                return _mc_rule_actor.ActByState(e, _state, s, assigned_cmds);
            });

        case ACTION_PROB:
            // parse probablity
            // [TODO] This code is really not good, need refactoring.
            {
              if (_receiver->GetUseCmdComment()) {
                string s;
                for (int i = 0; i < NUM_AISTATE; ++i) {
                    s += to_string(gs.pi[i]) + ",";
                }
                SendComment(s);
              }

              float pp[NUM_AISTATE + 1];
              float rd = float(rand() % 1000000) / 1000000;
              pp[0] = 0;
              for (int i = 1; i < num_action + 1; i++) {
                  pp[i] = gs.pi[i - 1] + pp[i - 1];
                  if (rd < pp[i]) {
                      h = i - 1;
                      break;
                  }
              }
              _state[h] = 1;
              return gather_decide(env, [&](const GameEnv &e, string *s, AssignedCmds *assigned_cmds) {
                  return _mc_rule_actor.ActByState(e, _state, s, assigned_cmds);
              });
            }

            /*
        case ACTION_REGIONAL:
            {
              if (_receiver->GetUseCmdComment()) {
                string s;
                for (size_t i = 0; i < gs.a_region.size(); ++i) {
                  for (size_t j = 0; j < gs.a_region[0].size(); ++j) {
                    int a = -1;
                    for (size_t k = 0; k < gs.a_region[0][0].size(); ++k) {
                      if (gs.a_region[k][i][j] == 1) {
                        a = k;
                        break;
                      }
                    }
                    s += to_string(a) + ",";
                  }
                  s += "; ";
                }
                SendComment(s);
              }
            }
            // Regional actions.
            return gather_decide(env, [&](const GameEnv &e, string *s, AssignedCmds *assigned_cmds) {
                return _mc_rule_actor.ActWithMap(e, reply.action_regions, s, assigned_cmds);
            });
            */
        default:
            throw std::range_error("action_type not valid! " + to_string(gs.action_type));
    }
}

///////////////////////////// Simple AI ////////////////////////////////
bool SimpleAI::on_act(const GameEnv &env) {
    _state.resize(NUM_AISTATE, 0);
    std::fill (_state.begin(), _state.end(), 0);
    return gather_decide(env, [&](const GameEnv &e, string *s, AssignedCmds *assigned_cmds) {
        _mc_rule_actor.GetActSimpleState(&_state);
        return _mc_rule_actor.ActByState(e, _state, s, assigned_cmds);
    });
}

bool HitAndRunAI::on_act(const GameEnv &env) {
    _state.resize(NUM_AISTATE, 0);
    std::fill (_state.begin(), _state.end(), 0);
    return gather_decide(env, [&](const GameEnv &e, string *s, AssignedCmds *assigned_cmds) {
        _mc_rule_actor.GetActHitAndRunState(&_state);
        return _mc_rule_actor.ActByState(e, _state, s, assigned_cmds);
    });
}
