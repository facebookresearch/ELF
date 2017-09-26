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
#include "engine/cmd_interface.h"

template <typename T>
void fill_zero(std::vector<T> &v) {
    std::fill(v.begin(), v.end(), (T)0);
}

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
    std::string player_name;

    // Extra data.
    int ai_start_tick;

    // Extracted feature map.
    std::vector<float> s;

    // Resource for each player (one-hot representation). Not used now.
    std::vector<float> res;

    float last_r;

    // Reply
    int32_t rv;

    int32_t action_type;
    int64_t a;
    float V;
    std::vector<float> pi;

    //
    std::vector<int64_t> uloc, tloc;
    std::vector<int64_t> bt, ct;

    // Also we need to save distributions.
    std::vector<float> uloc_prob, tloc_prob;
    std::vector<float> bt_prob, ct_prob;

    int n_max_cmd;
    int n_action;

    // Action as unit command.
    std::vector<CmdInput> unit_cmds;

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

    void Init(int iid, int num_action, int num_max_cmd, int mapx, int mapy, int num_ct, int num_units) {
        id = iid;
        pi.resize(num_action, 0.0);
        n_action = num_action;

        n_max_cmd = num_max_cmd;
        uloc.resize(num_max_cmd, 0);
        tloc.resize(num_max_cmd, 0);
        bt.resize(num_max_cmd, 0);
        ct.resize(num_max_cmd, 0);

        uloc_prob.resize(num_max_cmd * mapx * mapy, 0.0);
        tloc_prob.resize(num_max_cmd * mapx * mapy, 0.0);
        bt_prob.resize(num_max_cmd * num_units, 0.0);
        ct_prob.resize(num_max_cmd * num_ct, 0.0);
    }

    void Clear() {
        a = 0;
        V = 0.0;
        std::fill(pi.begin(), pi.end(), 0.0);
        unit_cmds.clear();

        fill_zero(uloc);
        fill_zero(tloc);
        fill_zero(bt);
        fill_zero(ct);

        fill_zero(uloc_prob);
        fill_zero(tloc_prob);
        fill_zero(bt_prob);
        fill_zero(ct_prob);

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

    bool AddUnitCmd(float uloc_x, float uloc_y, float tloc_x, float tloc_y, int ct, int build_tp) {
        if ((int)unit_cmds.size() < n_max_cmd) {
            unit_cmds.emplace_back(uloc_x, uloc_y, tloc_x, tloc_y, ct, build_tp);
            return true;
        } else return false;
    }

    // These fields are used to exchange with Python side using tensor interface.
    DECLARE_FIELD(GameState, id, a, V, pi, last_r, s, res, rv, terminal, seq, game_counter, last_terminal, uloc, tloc, bt, ct, uloc_prob, tloc_prob, bt_prob, ct_prob);
    REGISTER_PYBIND_FIELDS(id);
};

using Context = ContextT<PythonOptions, HistT<GameState>>;
