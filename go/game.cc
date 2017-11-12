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
#include "elf/tar_loader.h"

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

    _rw_buffer.reset(new elf::SharedRWBuffer("/home/yuandong/local/replay.db", "REPLAY"));
}

void GoGame::Init(AIComm *ai_comm) {
    assert(ai_comm);
    if (_options.mode == "online" || _options.mode == "selfplay") {
        if (_options.use_mcts) {
            auto *ai = new MCTSGoAI(ai_comm, _context_options.mcts_options);
            _ai.reset(ai);
        } else {
            auto *ai = new DirectPredictAI();
            ai->InitAIComm(ai_comm);
            ai->SetActorName("actor");
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

    if (_options.mode == "online") {
        HumanPlayer *player = new HumanPlayer;
        player->InitAIComm(ai_comm);
        player->SetActorName("human_actor");
        _human_player.reset(player);
    }

    if (_options.verbose) std::cout << "[" << _game_idx << "] Done with initialization" << std::endl;
}

void GoGame::Act(const elf::Signal &signal) {
    // Randomly pick one loader
    Coord c;
    if (_ai != nullptr) {
        // For human player, at least you need to run Act once.
        if (_human_player != nullptr) {
            do {
                _human_player->Act(_state, &c, &signal.done());
                if (_state.forward(c)) break;
                // cout << "Invalid move: x = " << X(c) << " y = " << Y(c) << " move: " << coord2str(c) << " please try again" << endl;
            } while(! signal.PrepareStop());
        }

        _ai->Act(_state, &c, &signal.done());
        if (! _state.forward(c) || _state.GetPly() > BOARD_SIZE * BOARD_SIZE ) {
            cout << _state.ShowBoard() << endl;
            cout << "No valid move [" << c << "][" << coord2str(c) << "][" << coord2str2(c) << "], restarting the game" << endl;
            _state.Reset();

            if (_tar_writer != nullptr) {
              _tar_writer->Write(std::to_string(_game_idx), coords2sgfstr(_moves));
            }
            if (_rw_buffer != nullptr) {
                elf::SharedRWBuffer::Record r;
                r.game_id = _game_idx;
                r.reward = _state.Evaluate([&]() -> unsigned int { return _rng(); });
                r.content = coords2sgfstr(_moves);
                _rw_buffer->Insert(r);
            }
            _moves.clear();
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
