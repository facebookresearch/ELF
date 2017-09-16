/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include "elf/pybind_helper.h"
#include "elf/comm_template.h"
#include "elf/hist.h"
#include "elf/copier.hh"

struct GameOptions {
    // Seed.
    unsigned int seed;
    int num_planes;
    int num_future_actions;

    // If true, then it will save the game to online mode.
    // In this mode, the thread will not output the next k moves (since every game is new), but will get the
    // action from the neural network to proceed.
    bool online = false;

    // A list file containing the files to load.
    std::string list_filename;
    bool verbose = false;

    REGISTER_PYBIND_FIELDS(seed, num_planes, num_future_actions, list_filename, verbose);
};

struct GameState {
    using State = GameState;
    // Board state 19x19
    // temp state.
    std::vector<float> features;

    // Next k actions.
    std::vector<int64_t> a;

    // Seq information.
    int32_t id = -1;
    int32_t seq = 0;
    int32_t game_counter = 0;
    char last_terminal = 0;

    int32_t move_idx = -1;
    int32_t winner = 0; // B +1, W -1, U 0

    std::string player_name;

    void Clear() { }

    void Init(int iid, int /*num_action*/) {
        id = iid;
    }

    GameState &Prepare(const SeqInfo &seq_info) {
        seq = seq_info.seq;
        game_counter = seq_info.game_counter;
        last_terminal = seq_info.last_terminal;

        Clear();
        return *this;
    }

    std::string PrintInfo() const {
        std::stringstream ss;
        ss << "[id:" << id << "][seq:" << seq << "][game_counter:" << game_counter << "][last_terminal:" << last_terminal << "]";
        return ss.str();
    }

    void Restart() {
        seq = 0;
        game_counter = 0;
        last_terminal = 0;
    }

    DECLARE_FIELD(GameState, id, seq, game_counter, last_terminal, features, a, move_idx, winner);
    REGISTER_PYBIND_FIELDS(id, seq, game_counter, last_terminal, features, a, move_idx, winner);
};

