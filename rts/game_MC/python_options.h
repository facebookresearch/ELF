/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once
#include "elf/copier.hh"
#include "elf/comm_template.h"
#include "elf/hist.h"
#include "engine/python_common_options.h"

#define ACTION_GLOBAL 0
#define ACTION_PROB 1
#define ACTION_REGIONAL 2
#define ACTION_UNIT_CMD 3 

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

    // Used for self-play.
    uint32_t player_id;
    std::string player_name;

    // Extra data.
    int ai_start_tick;

    // Extracted feature map.
    std::vector<float> s;

    // Resource for each player (one-hot representation).
    std::vector<float> res;

    float last_r;

    // Reply
    int64_t action_type;
    int32_t rv;

    int64_t a;
    float V;
    std::vector<float> pi;

    int n_max_cmd;
    int n_action;

    // Action as unit command.
    // nMaxCmd-by-2 (x, y)
    std::vector<float> unit_loc;
    std::vector<float> target_loc;

    // nMaxCmd (cmdType)
    std::vector<float> cmd_type;

    // nMaxCmd (buildType)
    std::vector<float> build_type;

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

    void Restart() { }

    void Init(int iid, int num_action, int num_max_cmd) {
        id = iid;
        pi.resize(num_action, 0.0);
        unit_loc.resize(num_max_cmd * 2, 0.0);
        target_loc.resize(num_max_cmd * 2, 0.0);
        cmd_type.resize(num_max_cmd, 0.0);
        build_type.resize(num_max_cmd, 0.0);
        n_action = num_action;
        n_max_cmd = num_max_cmd;
    }

    void Clear() {
        a = 0;
        V = 0.0;
        std::fill(pi.begin(), pi.end(), 0.0);
        action_type = ACTION_GLOBAL;

        /*
        // TODO Specify action map dimensions in Init.
        a_region.resize(20);
        for (size_t i = 0; i < action_regions.size(); ++i) {
            action_regions[i].resize(20);
            for (size_t j = 0; j < action_regions[i].size(); ++j) {
                action_regions[i][j].resize(20, 0);
            }
        }
        */
    }

    DECLARE_FIELD(GameState, id, a, V, pi, action_type, last_r, s, res, rv, terminal, seq, game_counter, last_terminal, unit_loc, target_loc, cmd_type, build_type);
    REGISTER_PYBIND_FIELDS(id, a, V, pi, action_type, last_r, s, res, tick, winner, player_id, ai_start_tick, last_terminal);
};

using Context = ContextT<PythonOptions, HistT<GameState>>;
