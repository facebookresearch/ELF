//File: game_context.h
//Author: Yuxin Wu <ppwwyyxxc@gmail.com>

#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

#include "game.h"
#include "../elf/pybind_interface.h"

class GameContext {
  public:
    using GC = ContextT<GameOptions, GameState, Reply>;

  private:
    std::unique_ptr<GC> _context;
    std::vector<GoGame> games;

    int _num_action = BOARD_DIM * BOARD_DIM;

  public:
    GameContext(const ContextOptions& context_options, const GameOptions& options) {
      _context.reset(new GC{context_options, options, CustomFieldFunc});
      for (int i = 0; i < context_options.num_games; ++i) {
        games.emplace_back(options);
      }
    }

    void Start() {
        auto f = [this](int game_idx, const GameOptions&,
                const std::atomic_bool& done, GC::AIComm* ai_comm) {
            auto& game = games[game_idx];
            game.initialize_comm(game_idx, ai_comm);
            game.MainLoop(&done);
        };
        _context->Start(f);
    }

    int get_num_actions() const { return _num_action; }

    CONTEXT_CALLS(GC, _context);

    void Stop() {
      _context.reset(nullptr); // first stop the threads, then destroy the games
      /*
      AtariGameSummary summary;
      for (const auto& game : games) {
          summary.Feed(game.summary());
      }
      std::cout << "Overall reward per game: ";
      summary.Print();*/
    }
};
