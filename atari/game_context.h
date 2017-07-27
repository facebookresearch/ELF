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

#include "atari_game.h"
#include "../elf/pybind_interface.h"

class GameContext {
  public:
    using GC = Context;

  private:
    std::unique_ptr<GC> _context;
    std::vector<AtariGame> games;

    int _width, _height, _num_action;

  public:
    GameContext(const ContextOptions& context_options, const GameOptions& options) {
      _context.reset(new GC{context_options, options, CustomFieldFunc});

      for (int i = 0; i < context_options.num_games; ++i) {
        games.emplace_back(options);
        if (i == 0) {
          auto& game = games.back();
          _width = game.width(), _height = game.height(), _num_action = game.num_actions();
          std::cout << "Action set: ";
          for (const auto &a : game.action_set()) {
              std::cout << a << " ";
          }
          std::cout << std::endl;
          // print more logs for the first game instance
          ale::Logger::setMode(ale::Logger::mode::Error);
        }
      }
    }

    void Start() {
        auto f = [this](int game_idx, const GameOptions&,
                const std::atomic_bool& done, GC::AIComm* ai_comm) {
            auto& game = games[game_idx];
            game.initialize_comm(game_idx, ai_comm);
            game.MainLoop(done);
        };
        _context->Start(f);
    }

    int get_screen_width() const { return _width; }
    int get_screen_height() const { return _height; }
    int get_num_actions() const { return _num_action; }

    CONTEXT_CALLS(GC, _context);

    void Stop() {
      _context.reset(nullptr); // first stop the threads, then destroy the games
      AtariGameSummary summary;
      for (const auto& game : games) {
          summary.Feed(game.summary());
      }
      std::cout << "Overall reward per game: ";
      summary.Print();
    }
};
