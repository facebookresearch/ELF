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
#include "elf/shared_replay_buffer.h"

#include "go_game_specific.h"
#include "go_state.h"
#include <random>
#include <map>

using Context = ContextT<GameOptions, HistT<GameState>>;
using Comm = typename Context::Comm;
using AIComm = AICommT<Comm>;

using RBuffer = SharedReplayBuffer<std::string, Sgf>;

// Game interface for Go.
class GoGame {
private:
    int _game_idx = -1;
    AIComm* _ai_comm = nullptr;
    GameOptions _options;

    std::mt19937 _rng;

    string _path;

    // Database
    vector<string> _games;

    // Current game, its sgf record and game board state.
    int _curr_game;
    GoState _state;

    // Shared Sgf buffer.
    RBuffer *_rbuffer = nullptr;

    void reload();
    const Sgf &pick_sgf();
    void print_context() const;

public:
    GoGame(int game_idx, const GameOptions& options);

    void Init(AIComm *ai_comm, RBuffer *rbuffer) {
        assert(ai_comm);
        assert(rbuffer);
        _ai_comm = ai_comm;
        _rbuffer = rbuffer;
    }

    void MainLoop(const std::atomic_bool& done) {
        // Main loop of the game.
        while (true) {
            Act(done);
            if (done.load()) break;
        }
    }

    void Act(const std::atomic_bool& done);
    string ShowBoard() const { return _state.ShowBoard(); }
};
