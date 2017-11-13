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

    // mode == "online": it will open the game in online mode.
    //    In this mode, the thread will not output the next k moves (since every game is new).
    //    Instead, it will get the action from the neural network to proceed.
    // mode == "offline": offline training
    // mode == "selfplay": self play.
    std::string mode;

    // Use mcts engine.
    bool use_mcts = false;

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
    std::string database_filename;
    bool verbose = false;

    std::string info() const {
        std::stringstream ss;
        ss << "Seed: " << seed << std::endl;
        ss << "#Plane: " << num_planes << std::endl;
        ss << "#FutureActions: " << num_future_actions << std::endl;
        ss << "#GamePerThread: " << num_games_per_thread << std::endl;
        ss << "mode: " << mode << std::endl;
        ss << "UseMCTS: " << elf_utils::print_bool(use_mcts) << std::endl;
        ss << "Data Aug: " << data_aug << std::endl;
        ss << "Start_ratio_pre_moves: " << start_ratio_pre_moves << std::endl;
        ss << "ratio_pre_moves: " << ratio_pre_moves << std::endl;
        ss << "MoveCutOff: " << move_cutoff << std::endl;
        ss << "ListFile: " << list_filename << std::endl;
        ss << "DBFile: " << database_filename << std::endl;
        ss << "Verbose: " << elf_utils::print_bool(verbose) << std::endl;
        return ss.str();
    }

    REGISTER_PYBIND_FIELDS(seed, mode, data_aug, start_ratio_pre_moves, ratio_pre_moves, move_cutoff, num_planes, num_future_actions, list_filename, verbose, num_games_per_thread, use_mcts, database_filename);
};

struct GameState {
    using State = GameState;
    // Board state BOARD_SIZE * BOARD_SIZE
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

    std::string name;

    int64_t a;
    std::vector<float> pi;
    float V; // B +1, W -1, U 0
    float winner;
    float last_r;

    void Clear() { game_record_idx = -1; aug_code = 0; last_r = 0.0; winner = 0.0; move_idx = -1; }

    void Init(int iid, int num_action) {
        id = iid;
        pi.resize(num_action, 0.0);
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

    DECLARE_FIELD(GameState, id, seq, game_counter, last_terminal, s, offline_a, a, V, pi, move_idx, aug_code, game_record_idx, last_r, winner);
    REGISTER_PYBIND_FIELDS(id, seq, game_counter, last_terminal, s, offline_a, a, V, move_idx, aug_code, last_r);
};

using Context = ContextT<GameOptions, HistT<GameState>>;
using Comm = typename Context::Comm;
using AIComm = AICommT<Comm>;

