/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "game.h"
#include "go_game_specific.h"
#include "offpolicy_loader.h"
#include "go_ai.h"
#include "mcts.h"

#include <fstream>

////////////////// GoGame /////////////////////
GoGame::GoGame(int game_idx, const ContextOptions &context_options, const GameOptions& options)
  : _options(options), _context_options(context_options), _curr_loader_idx(0) {
    _game_idx = game_idx;
    if (options.seed == 0) {
        auto now = chrono::system_clock::now();
        auto now_ms = chrono::time_point_cast<chrono::milliseconds>(now);
        auto value = now_ms.time_since_epoch();
        long duration = value.count();
        _seed = (time(NULL) * 1000 + duration + _game_idx * 2341479) % 100000000;
        if (_options.verbose) std::cout << "[" << _game_idx << "] Seed:" << _seed << std::endl;
    } else {
        _seed = options.seed;
    }
    _rng.seed(_seed);
}

void GoGame::Init(AIComm *ai_comm) {
    assert(ai_comm);
    if (_options.online) {
        if (_options.use_mcts) {
            auto *ai = new MCTSGoAI(ai_comm, _context_options.mcts_options);
            _ai.reset(ai);
        } else {
            auto *ai = new DirectPredictAI();
            ai->InitAIComm(ai_comm);
            _ai.reset(ai);
        }
    } else {
        // Open many offline instances.
        for (int i = 0; i < _options.num_games_per_thread; ++i) {
            auto *loader = new OfflineLoader(_options, _seed + _game_idx * i * 997 + i * 13773 + 7);
            loader->InitAIComm(ai_comm);
            _loaders.emplace_back(loader);
        }
    }
    if (_options.verbose) std::cout << "[" << _game_idx << "] Done with initialization" << std::endl;
}

void GoGame::Act(const std::atomic_bool& done) {
    // Randomly pick one loader
    Coord c;
    if (_options.online) {
        _ai->Act(_state, &c, &done);
        if (! _state.forward(c)) {
            cout << "Invalid move: x = " << X(c) << " y = " << Y(c) << coord2str(c) << " please try again" << endl;
        }
    } else {
        // Replays hold a state by itself.
        _curr_loader_idx = _rng() % _loaders.size();
        auto *loader = _loaders[_curr_loader_idx].get();
        loader->Act(&c, &done);
    }
}
