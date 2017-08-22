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

// Simulation type
#define ST_INVALID 0
#define ST_NORMAL 1

// AI type
#define AI_INVALID 0
#define AI_SIMPLE 1
#define AI_HIT_AND_RUN 2
#define AI_NN 3
#define AI_MCTS_VALUE 4
#define AI_FLAG_NN 5
#define AI_FLAG_SIMPLE 6
#define AI_TD_BUILT_IN 7
#define AI_TD_NN 8

#define ACTION_GLOBAL 0
#define ACTION_PROB 1
#define ACTION_REGIONAL 2

struct AIOptions {
    // Type of ai.
    int type;

    // Type of backup ai.
    int backup;

    // How often does the player acts.
    int fs;

    // Name of the player.
    std::string name;

    AIOptions() : type(AI_TD_BUILT_IN), backup(AI_INVALID), fs(1) {
    }

    std::string info() const {
        std::stringstream ss;
        ss << "[name=" << name << "][fs=" << fs << "][type=" << type << "][backup=" << backup << "]";
        return ss.str();
    }

    REGISTER_PYBIND_FIELDS(type, backup, fs, name);
};

struct PythonOptions {
    // What kind of simulations we are running.
    // For now there is only ST_NORMAL
    int simulation_type;

    // AI, backup_ai and its opponent type.
    std::vector<AIOptions> ai_options;

    // Send stdout to the following filename. If output_filename == "cout", send them to stdout.
    // If output_filename == "", disable the prompts.
    std::string output_filename;

    // When not empty string, set the prefix of the file used to dump each executing command (used for debugging).
    std::string cmd_dumper_prefix;

    // When not empty, save replays to the files.
    std::string save_replay_prefix;

    // Latest start of backup AI. When training, before each game starts,
    // we will sample a tick ~ Uniform(0, latest_start) and run backup AI
    // until that tick, then switch to NN-AI.
    int latest_start;

    // Decay of latest_start after each game.
    // After each game, latest_start is decayed by latest_start_decay.
    float latest_start_decay;

    // Max tick.
    int max_tick;

    // Random seed to use. seed = 0 mean uses time(NULL).
    // If seed != 0, then each simulation thread will use a seed which is a deterministic function
    // of PythonOption.seed and the thread id.
    int seed;

    bool shuffle_player;
    bool reverse_player;

    int mcts_threads;
    int mcts_rollout_per_thread;
    int game_name;
    int handicap_level;

    PythonOptions()
      : simulation_type(ST_NORMAL), latest_start(0),
      latest_start_decay(0.9), max_tick(30000), seed(0), shuffle_player(false), reverse_player(false), mcts_threads(1), mcts_rollout_per_thread(1),
      game_name(0), handicap_level(0) {
    }

    void AddAIOptions(const AIOptions &ai) {
        ai_options.push_back(ai);
    }

    void Print() const {
        std::cout << "Handicap: " << handicap_level << std::endl;
        std::cout << "Max tick: " << max_tick << std::endl;
        std::cout << "Latest_start: " << latest_start << " decay: " << latest_start_decay << std::endl;
        std::cout << "Seed: " << seed << std::endl;
        std::cout << "MCTS #threads: " << mcts_threads << " #rollout/thread: " << mcts_rollout_per_thread << std::endl;
        std::cout << "Output_prompt_filename: \"" << output_filename << "\"" << std::endl;
        std::cout << "Cmd_dumper_prefix: \"" << cmd_dumper_prefix << "\"" << std::endl;
        std::cout << "Save_replay_prefix: \"" << save_replay_prefix << "\"" << std::endl;
    }

    REGISTER_PYBIND_FIELDS(simulation_type, output_filename, cmd_dumper_prefix, save_replay_prefix, latest_start, latest_start_decay, max_tick, seed, mcts_threads, mcts_rollout_per_thread, game_name, handicap_level, shuffle_player, reverse_player);
};

struct GameState {
    using State = GameState;
    using Data = GameState;

    int32_t id;
    int32_t seq;
    int32_t game_counter;
    char terminal;

    int32_t tick;
    int32_t winner;
    int32_t player_id;
    std::string player_name;

    // Extra data.
    int ai_start_tick;

    // Extracted feature map.
    std::vector<float> s;

    float last_r;
    float base_hp_level;

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
        // last_terminal = seq_info.last_terminal;
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

    DECLARE_FIELD(GameState, a, V, pi, action_type, last_r, base_hp_level, s, rv, terminal, seq, game_counter);
    REGISTER_PYBIND_FIELDS(a, V, pi, action_type, last_r, base_hp_level, s, tick, winner, player_id, ai_start_tick);
};

using Context = ContextT<PythonOptions, HistT<GameState>>;
