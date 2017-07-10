/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once
#include "../elf/python_options_utils_cpp.h"

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

struct PythonOptions {
    // What kind of simulations we are running.
    // For now there is only ST_NORMAL
    int simulation_type;

    // AI, backup_ai and its opponent type.
    int ai_type;
    int backup_ai_type;
    int opponent_ai_type;

    // How often does the engine act (every act_ticks ticks).
    int frame_skip_ai, frame_skip_opponent;

    // Send stdout to the following filename. If output_filename == "cout", send them to stdout.
    // If output_filename == "", disable the prompts.
    std::string output_filename;

    // When not empty string, set the prefix of the file used to dump each executing command (used for debugging).
    std::string cmd_dumper_prefix;

    // When not empty, save replays to the files.
    std::string save_replay_prefix;

    // Two ratios to control mixture (by Qucheng?)
    float simple_ratio;
    float ratio_change;

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

    int mcts_threads;
    int mcts_rollout_per_thread;
    int game_name;
    int handicap_level;

    PythonOptions()
      : simulation_type(ST_NORMAL), ai_type(AI_SIMPLE), backup_ai_type(AI_SIMPLE), opponent_ai_type(AI_SIMPLE),
        frame_skip_ai(1), frame_skip_opponent(1), simple_ratio(1.0), ratio_change(0.0), latest_start(0),
        latest_start_decay(0.9), max_tick(30000), seed(0), mcts_threads(1), mcts_rollout_per_thread(1),
        game_name(0), handicap_level(0) {
    }

    void Print() const {
        std::cout << "Frame_skip_ai: " << frame_skip_ai << " Frame_skip_opponent: " << frame_skip_opponent << std::endl;
        std::cout << "AI type: " << ai_type << std::endl;
        std::cout << "Opponent AI type: " << opponent_ai_type << std::endl;
        std::cout << "Backup AI type: " << backup_ai_type << std::endl;
        std::cout << "Handicap: " << handicap_level << std::endl;
        std::cout << "Max tick: " << max_tick << std::endl;
        std::cout << "Latest_start: " << latest_start << " decay: " << latest_start_decay << std::endl;
        std::cout << "Seed: " << seed << std::endl;
        std::cout << "MCTS #threads: " << mcts_threads << " #rollout/thread: " << mcts_rollout_per_thread << std::endl;
        std::cout << "Output_prompt_filename: \"" << output_filename << "\"" << std::endl;
        std::cout << "Cmd_dumper_prefix: \"" << cmd_dumper_prefix << "\"" << std::endl;
        std::cout << "Save_replay_prefix: \"" << save_replay_prefix << "\"" << std::endl;
    }

    REGISTER_PYBIND_FIELDS(simulation_type, ai_type, backup_ai_type, opponent_ai_type, frame_skip_ai, frame_skip_opponent, output_filename, cmd_dumper_prefix, save_replay_prefix, simple_ratio, ratio_change, latest_start, latest_start_decay, max_tick, seed, mcts_threads, mcts_rollout_per_thread, game_name, handicap_level);
};

struct ExtGame {
    int tick;
    int winner;
    bool terminated;
    int player_id;

    // Extra data.
    int ai_start_tick;

    // Extracted feature map.
    std::vector<float> features;

    // Resource for each player (one-hot representation).
    std::vector<std::vector<float>> resources;

    float last_reward;
};

struct Reply {
    int action_type;

    int global_action;
    float value;
    std::vector<float> action_probs;

    // Action per region
    // Python side will output an action map for each region for the player to follow.
    std::vector<std::vector<std::vector<int>>> action_regions;

    void Clear() {
        global_action = 0;
        action_type = ACTION_GLOBAL;
        action_probs.resize(20, 0);
        action_regions.resize(20);
        for (size_t i = 0; i < action_regions.size(); ++i) {
            action_regions[i].resize(20);
            for (size_t j = 0; j < action_regions[i].size(); ++j) {
                action_regions[i][j].resize(20, 0);
            }
        }
    }
};
