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

#include "go_game_specific.h"
#include "go_state.h"
#include <random>
#include <map>

using Context = ContextT<GameOptions, HistT<GameState>>;
using Comm = typename Context::Comm;
using AIComm = AICommT<Comm>;

// Game interface for Go.
class GoGame {
private:
    int _game_idx = -1;
    AIComm* _ai_comm = nullptr;
    GameOptions _options;

    std::vector<std::unique_ptr<Loader>> _loaders;
    int _curr_loader_idx;
    std::mt19937 _rng;

public:
    GoGame(int game_idx, const GameOptions& options);

    void Init(AIComm *ai_comm) {
        assert(ai_comm);
        _ai_comm = ai_comm;
    }

    void MainLoop(const std::atomic_bool& done) {
        // Main loop of the game.
        while (true) {
            Act(done);
            if (done.load()) break;
        }
    }

    void Act(const std::atomic_bool& done);
    string ShowBoard() const { return _loaders[_curr_loader_idx]->state().ShowBoard(); }
    void UndoMove() {_loaders[_curr_loader_idx]->UndoMove();}
    void ApplyHandicap(int handicap) {_loaders[_curr_loader_idx]->ApplyHandicap(handicap);}
};
