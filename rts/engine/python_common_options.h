/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once
#include <sstream>
#include "elf/python_options_utils_cpp.h"

// Simulation type
#define ST_INVALID 0
#define ST_NORMAL 1

struct AIOptions {
    // Type of ai.
    std::string type;

    // How often does the player acts.
    int fs;

    // Name of the player.
    std::string name;

    // Whether it respects FoW.
    bool fow;

    // other args.
    std::string args;

    AIOptions() : fs(1), fow(true) {
    }

    std::string info() const {
        std::stringstream ss;
        ss << "[name=" << name << "][fs=" << fs << "][type=" << type << "][FoW=" << (fow ? "True" : "False") << "]";
        if (! args.empty()) ss << "[args=" << args << "]"; 
        return ss.str();
    }

    REGISTER_PYBIND_FIELDS(type, fs, name, fow, args);
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

    // Map size 
    int map_size_x, map_size_y;

    // Maximum unit command you could send per action.
    int max_unit_cmd;

    // Max tick.
    int max_tick;

    // Random seed to use. seed = 0 mean uses time(NULL).
    // If seed != 0, then each simulation thread will use a seed which is a deterministic function
    // of PythonOption.seed and the thread id.
    int seed;

    bool shuffle_player;

    int mcts_threads;
    int mcts_rollout_per_thread;
    int game_name;
    // [TODO] put handicap to TD.
    int handicap_level;

    PythonOptions()
      : simulation_type(ST_NORMAL), map_size_x(20), map_size_y(20), max_unit_cmd(10), max_tick(30000), seed(0), shuffle_player(false), mcts_threads(1), mcts_rollout_per_thread(1),
        game_name(0), handicap_level(0) {
    }

    void AddAIOptions(const AIOptions &ai) {
        ai_options.push_back(ai);
    }

    void Print() const {
        std::cout << "Map: " << map_size_x << " by " << map_size_y << std::endl;
        std::cout << "Handicap: " << handicap_level << std::endl;
        std::cout << "Max tick: " << max_tick << std::endl;
        std::cout << "Max #Unit Cmd: " << max_unit_cmd << std::endl;
        std::cout << "Seed: " << seed << std::endl;
        std::cout << "Shuffled: " << (shuffle_player ? "True" : "False") << std::endl;
        for (const AIOptions& ai_option : ai_options) {
            std::cout << ai_option.info() << std::endl;
        }
        std::cout << "MCTS #threads: " << mcts_threads << " #rollout/thread: " << mcts_rollout_per_thread << std::endl;
        std::cout << "Output_prompt_filename: \"" << output_filename << "\"" << std::endl;
        std::cout << "Cmd_dumper_prefix: \"" << cmd_dumper_prefix << "\"" << std::endl;
        std::cout << "Save_replay_prefix: \"" << save_replay_prefix << "\"" << std::endl;
    }

    REGISTER_PYBIND_FIELDS(simulation_type, map_size_x, map_size_y, max_unit_cmd, output_filename, cmd_dumper_prefix, save_replay_prefix, max_tick, seed, mcts_threads, mcts_rollout_per_thread, game_name, handicap_level, shuffle_player);
};
