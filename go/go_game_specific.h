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
    int num_future_actions = 3;

    // How many games to open simultaneously per thread?
    // When sending current situations, randomly select one to break any correlations.
    int num_games_per_thread = 300;

    // If true, then it will open the game in online mode.
    // In this mode, the thread will not output the next k moves (since every game is new).
    // Instead, it will get the action from the neural network to proceed.
    bool online = false;

    // -1 is random, 0-7 mean specific data aug.
    int data_aug = -1;

    // Before we use the data from the first recorded replay in each thread, sample number of moves to fast-forward.
    // This is to increase the diversity of the game.
    // This number is sampled uniformly from [0, #Num moves * ratio_pre_moves]
    float start_ratio_pre_moves = 0.5;

    // Similar as above, but note that it will be applied to any newly loaded game (except for the first one).
    // This will introduce bias in the dataset.
    // Useful when you want that (or when you want to visualize the data).
    float ratio_pre_moves = 0.0;

    // Cutoff ply for each loaded game, and start a new one.
    int move_cutoff = -1;

    // A list file containing the files to load.
    std::string list_filename;
    bool verbose = false;

    REGISTER_PYBIND_FIELDS(seed, online, data_aug, start_ratio_pre_moves, ratio_pre_moves, move_cutoff, num_planes, num_future_actions, list_filename, verbose, num_games_per_thread);
};

struct GameState {
    using State = GameState;
    // Board state 19x19
    std::vector<float> s;

    // Next k actions.
    std::vector<int64_t> offline_a;

    // Seq information.
    int32_t id = -1;
    int32_t seq = 0;
    int32_t game_counter = 0;
    char last_terminal = 0;

    int32_t game_record_idx = -1;
    int32_t move_idx = -1;
    int32_t aug_code = 0;
    int32_t winner = 0; // B +1, W -1, U 0

    int64_t a;
    std::vector<float> pi;
    float V;

    std::string player_name;

    void Clear() { game_record_idx = -1; aug_code = 0; winner = 0; move_idx = -1; }

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

    DECLARE_FIELD(GameState, id, seq, game_counter, last_terminal, s, offline_a, a, V, pi, move_idx, winner, aug_code, game_record_idx);
    REGISTER_PYBIND_FIELDS(id, seq, game_counter, last_terminal, s, offline_a, a, V, move_idx, winner, aug_code);
};

using Context = ContextT<GameOptions, HistT<GameState>>;
using Comm = typename Context::Comm;
using AIComm = AICommT<Comm>;

