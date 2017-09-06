/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

//File: game_context.h
//Author: Yuxin Wu <ppwwyyxxc@gmail.com>

#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

#include "game.h"
#include "../elf/pybind_interface.h"

class GameContext {
  public:
    using GC = Context;

  private:
    std::unique_ptr<GC> _context;
    std::vector<GoGame> _games;
    const int _num_action = BOARD_DIM * BOARD_DIM;

    std::unique_ptr<RBuffer> _shared_buffer;

  public:
    GameContext(const ContextOptions& context_options, const GameOptions& options) {
      _context.reset(new GC{context_options, options});
      for (int i = 0; i < context_options.num_games; ++i) {
          _games.emplace_back(i, options);
      }

      _shared_buffer.reset(new RBuffer([](const std::string &name) {
            std::unique_ptr<Sgf> sgf(new Sgf());
            sgf->Load(name);
            return sgf;
      }));
    }

    void Start() {
        RBuffer *rbuffer = _shared_buffer.get();

        auto f = [this, rbuffer](int game_idx, const ContextOptions &context_options, const GameOptions&,
                const std::atomic_bool& done, GC::Comm* comm) {
            GC::AIComm ai_comm(game_idx, comm);
            auto &state = ai_comm.info().data;
            state.InitHist(context_options.T);
            for (auto &s : state.v()) {
                s.Init(game_idx, _num_action);
            }
            auto& game = _games[game_idx];
            game.Init(&ai_comm, rbuffer);
            game.MainLoop(done);
        };
        _context->Start(f);
    }

    std::map<std::string, int> GetParams() const {
        return std::map<std::string, int>{
          { "num_action", _num_action },
          { "board_size", BOARD_SIZE },
          { "num_planes", MAX_NUM_FEATURE },
          { "num_future_actions", NUM_FUTURE_ACTIONS}
        };
    }

    EntryInfo EntryFunc(const std::string &key) {
        auto *mm = GameState::get_mm(key);
        if (mm == nullptr) return EntryInfo();

        std::string type_name = mm->type();

        if (key == "features") return EntryInfo(key, type_name, {MAX_NUM_FEATURE, BOARD_DIM, BOARD_DIM});
        else if (key == "a") return EntryInfo(key, type_name, {NUM_FUTURE_ACTIONS});
        else if (key == "last_terminal" || key == "id" || key == "seq" || key == "game_counter") return EntryInfo(key, type_name);
        else if (key == "move_idx") return EntryInfo(key, type_name);
        else if (key == "winner") return EntryInfo(key, type_name);

        return EntryInfo();
    }

    std::string ShowBoard(int game_idx) const {
        if (game_idx < 0 || game_idx >= (int)_games.size()) return "Invalid game_idx [" + std::to_string(game_idx) + "]";
        return _games[game_idx].ShowBoard();
    }

    CONTEXT_CALLS(GC, _context);

    void Stop() {
      _context.reset(nullptr);
      // [TODO] there may be issues when deleting shared_buffer.
    }
};
