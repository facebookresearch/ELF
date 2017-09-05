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
    std::vector<GoGame> games;

    int _num_action = BOARD_DIM * BOARD_DIM;

  public:
    GameContext(const ContextOptions& context_options, const GameOptions& options) {
      _context.reset(new GC{context_options, options});
      for (int i = 0; i < context_options.num_games; ++i) {
        games.emplace_back(options);
      }
    }

    void Start() {
        auto f = [this](int game_idx, const ContextOptions &context_options, const GameOptions&,
                const std::atomic_bool& done, GC::Comm* comm) {
            GC::AIComm ai_comm(game_idx, comm);
            auto &state = ai_comm.info().data;
            state.InitHist(context_options.T);
            for (auto &s : state.v()) {
                s.Init(game_idx, _num_action);
            }
            auto& game = games[game_idx];
            game.initialize_comm(game_idx, &ai_comm);
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

        return EntryInfo();
    }

    CONTEXT_CALLS(GC, _context);

    void Stop() {
      _context.reset(nullptr);
    }
};
