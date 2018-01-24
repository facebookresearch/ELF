/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "game_selfplay.h"
#include "offpolicy_loader.h"

#include "go_game_specific.h"
#include "go_ai.h"
#include "mcts.h"

#include <fstream>

////////////////// GoGame /////////////////////
GoGameSelfPlay::GoGameSelfPlay(int game_idx, elf::SharedRWBuffer *rw_buffer, const ContextOptions &context_options, const GameOptions& options)
  : _options(options), _context_options(context_options), _rw_buffer(rw_buffer) {
    _game_idx = game_idx;
    if (options.seed == 0) {
        _seed = elf_utils::get_seed(game_idx);
        if (_options.verbose) std::cout << "[" << _game_idx << "] Seed:" << _seed << std::endl;
    } else {
        _seed = options.seed;
    }
    _rng.seed(_seed);
}

void GoGameSelfPlay::Init(AIComm *ai_comm) {
    assert(ai_comm);
    _ai_comm = ai_comm;
    if (_options.mode == "selfplay") {
        if (_options.use_mcts) {
            auto *ai = new MCTSGoAI(ai_comm, _context_options.mcts_options);
            _ai.reset(ai);
        } else {
            auto *ai = new DirectPredictAI();
            ai->InitAIComm(ai_comm);
            ai->SetActorName("actor");
            _ai.reset(ai);
        }
    } else if (_options.mode == "train") {
        // Open many offline instances.
        for (int i = 0; i < _options.num_games_per_thread; ++i) {
            auto *loader = new OfflineLoader(_options, _seed + _game_idx * i * 997 + i * 13773 + 7);
            loader->InitAIComm(ai_comm);
            _loaders.emplace_back(loader);
        }
    } else {
        std::cout << "Unknown mode! " << _options.mode << std::endl;
        throw std::range_error("Unknown mode");
    }

    if (_options.verbose) std::cout << "[" << _game_idx << "] Done with initialization" << std::endl;
}

void GoGameSelfPlay::Act(const elf::Signal &signal) {
    // Randomly pick one loader
    Coord c;
    if (_ai != nullptr) {

        _ai->Act(_state, &c, &signal.done());
        if (! _state.forward(c) || _state.GetPly() > BOARD_SIZE * BOARD_SIZE ) {
            cout << _state.ShowBoard() << endl;
            cout << "No valid move [" << c << "][" << coord2str(c) << "][" << coord2str2(c) << "], ";
            cout << "or ply: " << _state.GetPly() << " exceeds threads.Restarting the game" << endl;

            if (_rw_buffer != nullptr) {
                elf::SharedRWBuffer::Record r;
                r.game_id = _game_idx;
                r.reward = _state.Evaluate([&]() -> unsigned int { return _rng(); });
                r.content = coords2sgfstr(_moves);
                int num_inserted = _rw_buffer->Insert(r);
                if (num_inserted < 0) {
                    cout << "Insert error! Last error: " << _rw_buffer->LastError() << endl;
                } else {
                    cout << "Inserted " << num_inserted << " entries" << endl;
                }
            }

            _state.Reset();
            _moves.clear();
            _ai->GameEnd();
            _game_idx++;
        } else {
          _moves.push_back(c);
        }
    } else {
        // Replays hold a state by itself.
        _curr_loader_idx = _rng() % _loaders.size();
        auto *loader = _loaders[_curr_loader_idx].get();
        loader->Act(&c, &signal.done());
    }
}
