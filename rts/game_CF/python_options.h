/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once
#include "elf/python_options_utils_cpp.h"
#include "elf/copier.hh"
#include "elf/comm_template.h"
#include "elf/hist.h"
#include "engine/python_common_options.h"


#define ACTION_GLOBAL 0
#define ACTION_PROB 1
#define ACTION_REGIONAL 2

struct GameState {
    using State = GameState;
    using Data = GameState;

    int32_t id;
    int32_t seq;
    int32_t game_counter;
    char terminal;
    char last_terminal;

    int32_t tick;
    int32_t winner;
    int32_t player_id;
    std::string player_name;

    // Extra data.
    int ai_start_tick;

    // Extracted feature map.
    std::vector<float> s;

    // Resource for each player (one-hot representation).
    std::vector<float> res;

    float last_r;
    int r0;
    int r1;
    int flag_x;

    // Reply
    int64_t action_type;
    int32_t rv;

    int64_t a;
    float V;
    std::vector<float> pi;

    // Action per region
    // Python side will output an action map for each region for the player to follow.
    // std::vector<std::vector<std::vector<int>>> a_region;

    GameState &Prepare(const SeqInfo &seq_info) {
        seq = seq_info.seq;
        game_counter = seq_info.game_counter;
        last_terminal = seq_info.last_terminal;
        Clear();
        return *this;
    }

    void Restart() {

    }

    void Init(int iid, int num_action) {
        id = iid;
        pi.resize(num_action, 0.0);
    }

    void Clear() {
        a = 0;
        V = 0.0;
        std::fill(pi.begin(), pi.end(), 0.0);
        action_type = ACTION_GLOBAL;
    }

    DECLARE_FIELD(GameState, id, a, V, pi, action_type, last_r, r0, r1, flag_x, s, res, rv, terminal, seq, game_counter, last_terminal);
    REGISTER_PYBIND_FIELDS(id, a, V, pi, action_type, last_r, r0, r1, flag_x, s, res, tick, winner, player_id, ai_start_tick, last_terminal);
};

using Context = ContextT<PythonOptions, HistT<GameState>>;
