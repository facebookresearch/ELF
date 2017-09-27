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
#include "engine/cmd_interface.h"
#include "python_options.h"

#define _OFFSET(_c, _x, _y, _m) (((_c) * _m.GetYSize() + (_y)) * _m.GetXSize() + (_x))
#define _XY(loc, m) ((loc) % m.GetXSize()), ((loc) / m.GetXSize())

/*
static inline int sampling(const std::vector<float> &v, std::mt19937 *gen) {
    std::vector<float> accu(v.size() + 1);
    std::uniform_real_distribution<> dis(0, 1);
    float rd = dis(*gen);

    accu[0] = 0;
    for (size_t i = 1; i < accu.size(); i++) {
        accu[i] = v[i - 1] + accu[i - 1];
        if (rd < accu[i]) {
            return i - 1;
        }
    }

    return v.size() - 1;
}
*/

static inline void accu_value(int idx, float val, std::map<int, std::pair<int, float> > &idx2record) {
    auto it = idx2record.find(idx);
    if (it == idx2record.end()) {
        idx2record.insert(make_pair(idx, make_pair(1, val)));
    } else {
        it->second.second += val;
        it->second.first ++;
    }
}

bool TrainedAI::GameEnd(Tick t) {
    AIWithComm::GameEnd(t);
    for (auto &v : _recent_states.v()) {
        v.clear();
    }
    return true;
}

void TrainedAI::extract(Data *data) {
    const GameEnv &env = s().env();

    GameState *game = &data->newest();
    game->tick = s().receiver().GetTick();
    game->winner = env.GetWinnerId();
    game->terminal = env.GetTermination() ? 1 : 0;
    game->player_name = name();

    // Extra data.
    game->ai_start_tick = 0;

    if (_recent_states.maxlen() == 1) {
        compute_state(&game->s);
        // std::cout << "(1) size_s = " << game->s.size() << std::endl << std::flush;
    } else {
        std::vector<float> &state = _recent_states.GetRoom();
        compute_state(&state);

        const size_t maxlen = _recent_states.maxlen();
        game->s.resize(maxlen * state.size());
        // std::cout << "(" << maxlen << ") size_s = " << game->s.size() << std::endl << std::flush;
        std::fill(game->s.begin(), game->s.end(), 0.0);
        // Then put all states to game->s.
        for (size_t i = 0; i < maxlen; ++i) {
            const auto &curr_s = _recent_states.get_from_push(i);
            if (! curr_s.empty()) {
                assert(curr_s.size() == state.size());
                std::copy(curr_s.begin(), curr_s.end(), &game->s[i * curr_s.size()]);
            }
        }
    }

    // res is not used.
    game->res.resize(s().env().GetNumOfPlayers() * NUM_RES_SLOT, 0.0);

    game->last_r = 0.0;
    int winner = s().env().GetWinnerId();

    if (winner != INVALID) {
        if (winner == id()) game->last_r = 1.0;
        else game->last_r = -1.0;
    }
}

void TrainedAI::compute_state(std::vector<float> *state) {
    const GameEnv &env = s().env();

    const int n_type = env.GetGameDef().GetNumUnitType();
    const int n_additional = 2;
    const int resource_grid = 50;
    const int res_pt = NUM_RES_SLOT;
    const int total_channel = n_type + n_additional + res_pt;

    const auto &m = env.GetMap();

    // [Channel, width, height]
    const int sz = total_channel * m.GetXSize() * m.GetYSize();
    state->resize(sz);
    std::fill(state->begin(), state->end(), 0.0);

    std::map<int, std::pair<int, float> > idx2record;

    PlayerId visibility_check = _respect_fow ? id() : INVALID;

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

        bool self_unit = (u.GetPlayerId() == id());

        accu_value(_OFFSET(t, x, y, m), 1.0, idx2record);

        // Self unit or enemy unit.
        // For historical reason, the flag of enemy unit = 2
        accu_value(_OFFSET(n_type, x, y, m), (self_unit ? 1 : 2), idx2record);
        accu_value(_OFFSET(n_type + 1, x, y, m), hp_level, idx2record);

        total_hp_ratio += hp_level;

        if (self_unit) {
            if (t == WORKER) myworker += 1;
            else if (t == MELEE_ATTACKER || t == RANGE_ATTACKER) mytroop += 1;
            else if (t == BARRACKS) mybarrack += 1;
       }

        ++ unit_iter;
    }

    for (const auto &p : idx2record) {
        state->at(p.first) = p.second.second / p.second.first;
    }

    myworker = min(myworker, 3);
    mytroop = min(mytroop, 5);
    mybarrack = min(mybarrack, 1);

    for (int i = 0; i < env.GetNumOfPlayers(); ++i) {
        // Omit player signal from other player's perspective.
        if (visibility_check != INVALID && visibility_check != i) continue;
        const auto &player = env.GetPlayer(i);
        quantized_r[i] = min(int(player.GetResource() / resource_grid), res_pt - 1);
    }

    if (id() != INVALID) {
        // Add resource layer for the current player.
        const int c = _OFFSET(n_type + n_additional + quantized_r[id()], 0, 0, m);
        std::fill(state->begin() + c, state->begin() + c + m.GetXSize() * m.GetYSize(), 1.0);
    }
}

#define ACTION_GLOBAL 0
#define ACTION_UNIT_CMD 1
#define ACTION_REGIONAL 2

bool TrainedAI::handle_response(const Data &data, RTSMCAction *a) { 
    a->Init(id(), name());

    // if (_receiver == nullptr) return false;
    const auto &env = s().env();

    // Get the current action from the queue.
    const auto &m = env.GetMap();
    const GameState& gs = data.newest();

    switch(gs.action_type) {
        case ACTION_GLOBAL:
            // action
            a->SetState9(gs.a);
            break;

        case ACTION_UNIT_CMD:
            {
                // Use gs.unit_cmds
                // std::vector<CmdInput> unit_cmds(gs.unit_cmds);
                // Use data
                std::vector<CmdInput> unit_cmds;
                for (int i = 0; i < gs.n_max_cmd; ++i) {
                    unit_cmds.emplace_back(_XY(gs.uloc[i], m), _XY(gs.tloc[i], m), gs.ct[i], gs.bt[i]);
                }
                std::for_each(unit_cmds.begin(), unit_cmds.end(), [&](CmdInput &ci) { ci.ApplyEnv(env); });
                a->SetUnitCmds(unit_cmds);
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
            return _mc_rule_actor.ActWithMap(env,reply.action_regions, &a->state_string(), &a->cmds());
            */
        default:
            throw std::range_error("action_type not valid! " + to_string(gs.action_type));
    }
    return true;
}

///////////////////////////// Simple AI ////////////////////////////////
bool SimpleAI::on_act(Tick, RTSMCAction *a, const std::atomic_bool *) {
    a->Init(id(), name());
    a->SetSimpleAI();
    return true;
}

bool HitAndRunAI::on_act(Tick, RTSMCAction *a, const std::atomic_bool *) {
    a->Init(id(), name());
    a->SetHitAndRunAI();
    return true;
}

////////////////////// MCTS AI //////////////////////
bool MCTSAI::on_act(Tick t, RTSMCAction *a, const std::atomic_bool *done) {
    _ts
}
