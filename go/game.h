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
#include "elf/ai_comm.h"

#include "ai.h"
#include "go_game_specific.h"
#include "go_state.h"
#include "offpolicy_loader.h"
#include <random>
#include <map>

// Game interface for Go.
class GoGame {
private:
    int _game_idx = -1;
    uint64_t _seed = 0;
    GameOptions _options;
    ContextOptions _context_options;

    std::vector<std::unique_ptr<OfflineLoader>> _loaders;
    int _curr_loader_idx;
    std::mt19937 _rng;

    std::unique_ptr<AI> _ai;
    std::unique_ptr<AI> _human_player;

    // Only used when we want to run online
    GoState _state;

    std::vector<Coord> _moves;
    std::unique_ptr<elf::tar::TarWriter> _tar_writer;

public:
    GoGame(int game_idx, const ContextOptions &context_options, const GameOptions& options);

    void Init(AIComm *ai_comm);

    void MainLoop(const elf::Signal& signal) {
        // Main loop of the game.
        while (! signal.IsDone()) {
            Act(signal);
        }
    }

    void Act(const elf::Signal &signal);
    string ShowBoard() const { return _state.ShowBoard(); }
};
