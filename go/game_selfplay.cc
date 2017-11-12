/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "game_selfplay.h"
#include "go_game_specific.h"
#include "go_ai.h"
#include "mcts.h"

#include <fstream>

////////////////// GoGame /////////////////////
GoGameSelfPlay::GoGameSelfPlay(int game_idx, const ContextOptions &context_options, const GameOptions& options)
  : _options(options), _context_options(context_options) {
    _game_idx = game_idx;
    if (options.seed == 0) {
        _seed = elf_utils::get_seed(game_idx);
        if (_options.verbose) std::cout << "[" << _game_idx << "] Seed:" << _seed << std::endl;
    } else {
        _seed = options.seed;
    }
    _rng.seed(_seed);

    _rw_buffer.reset(new elf::SharedRWBuffer("/home/yuandong/local/replay.db", "REPLAY"));
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
    } else if (_options.mode == "training") {
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

            elf::SharedRWBuffer::Record r;
            r.game_id = _game_idx;
            r.reward = _state.Evaluate([&]() -> unsigned int { return _rng(); });
            r.content = coords2sgfstr(_moves);
            if (! _rw_buffer->Insert(r)) {
                cout << "Insert error! Last error: " << _rw_buffer->LastError() << endl;
            }

            _state.Reset();
            _moves.clear();
            _game_idx++;
        } else {
          _moves.push_back(c);
        }
    } else {
        // Train a model directly.
        auto sampler = _rw_buffer->GetSampler();
        const elf::SharedRWBuffer::Record &r = sampler.sample();
        vector<Coord> moves = sgfstr2coords(r.content);

        // Random sample one move
        int move_to = _rng() % (moves.size() - _options.num_future_actions + 1);
        _state.Reset();
        for (int i = 0; i < move_to; ++i) {
            _state.forward(moves[i]);
        }

        // Then send the data to the server.
        auto &gs = _ai_comm->Prepare();
        gs.move_idx = _state.GetPly();
        gs.winner = r.reward;

        int code = _rng() % 8;
        gs.aug_code = code;
        const BoardFeature &bf = _state.extractor(code);
        bf.Extract(&gs.s);

        gs.offline_a.resize(_options.num_future_actions);
        for (int i = 0; i < _options.num_future_actions; ++i) {
            gs.offline_a[i] = bf.Coord2Action(moves[move_to + i]);
        }
        _ai_comm->SendDataWaitReply();
    }
}
