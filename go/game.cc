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
#include "../elf/tar_loader.h"

#include <fstream>

////////////////// GoGame /////////////////////
GoGame::GoGame(int game_idx, const GameOptions& options) : _options(options) {
    _game_idx = game_idx;
    uint64_t seed = 0;
    if (options.seed == 0) {
        auto now = chrono::system_clock::now();
        auto now_ms = chrono::time_point_cast<chrono::milliseconds>(now);
        auto value = now_ms.time_since_epoch();
        long duration = value.count();
        seed = (time(NULL) * 1000 + duration + _game_idx * 2341479) % 100000000;
        if (_options.verbose) std::cout << "[" << _game_idx << "] Seed:" << seed << std::endl;
    } else {
        seed = options.seed;
    }
    _rng.seed(seed);

    if (_options.verbose) std::cout << "[" << _game_idx << "] Loading list_file: " << options.list_filename << std::endl;
    if (file_is_tar(options.list_filename)) {
      // Get all .sgf from tar
      TarLoader tl = TarLoader(options.list_filename.c_str());
      _games = tl.List();
    } else {
      // Get all .sgf file in the directory.
      ifstream iFile(options.list_filename);
      _games.clear();
      for (string this_game; std::getline(iFile, this_game) ; ) {
        _games.push_back(this_game);
      }
      while (_games.back().empty()) _games.pop_back();

      // Get the path of the filename.
      _path = string(options.list_filename);
      int i = _path.size() - 1;
      while (_path[i] != '/' && i >= 0) i --;

      if (i >= 0) _path = _path.substr(0, i + 1);
      else _path = "";
    }
    if (_options.verbose) std::cout << "[" << _game_idx << "] Loaded: #Game: " << _games.size() << std::endl;

    if (_options.verbose) std::cout << "[" << _game_idx << "] Done with initialization" << std::endl;
}

void GoGame::print_context() const {
    cout << "[id=" << _game_idx << "][curr_game=" << _curr_game << "][filename=" << _games[_curr_game] << " " << _state.info() << endl;
}

void GoGame::Act(const std::atomic_bool& done) {
  // Act on the current game.
  while ( _state.NeedReload() && !done.load() ) {
      // std::cout << "Reloading games.." << std::endl;
      reload();
  }
  if (done.load()) return;

  vector<SgfMove> future_moves;
  while (true) {
      if (_state.GetForwardMoves(&future_moves)) break;
      print_context();
      cout << "future_moves.size() [" +
          std::to_string(future_moves.size()) + "] < #FUTURE_ACTIONS [" + std::to_string(NUM_FUTURE_ACTIONS) << endl;
      reload();
  }
  //bool terminal = (_sgf.StepLeft() == NUM_FUTURE_ACTIONS);

  // Send the current board situation.
  auto& gs = _ai_comm->Prepare();
  //meta->tick = _board._ply;
  //meta->terminated = terminal;
  //if (terminal) meta->winner = _sgf.GetWinner();
  _state.SaveTo(gs, future_moves, _rng);

  // There is always only 1 player.
  _ai_comm->SendDataWaitReply();

  if (!_state.NextMove()) reload();
}

const Sgf &GoGame::pick_sgf() {
    while (true) {
        _curr_game = _rng() % _games.size();
        // std::cout << "_game_idx = " << _curr_game << std::endl;

        std::string full_name = file_is_tar(_options.list_filename) ?
           _games[_curr_game] : _path + _games[_curr_game];
        // std::cout << "full_name = " << full_name << std::endl;

        bool file_loaded = _rbuffer->HasKey(full_name);
        // std::cout << "Has key: " << (file_loaded ? "True" : "False") << std::endl;

        const auto &sgf = _rbuffer->Get(full_name);
        if (_options.verbose) {
            if (! file_loaded)
              std::cout << "Loaded file " << full_name << std::endl;
        }
        if (sgf.NumMoves() >= 10 && sgf.GetBoardSize() == BOARD_DIM) return sgf;
    }
}

void GoGame::reload() {
    const Sgf &sgf = pick_sgf();
    _state.Reset(sgf);

    if (_options.verbose) print_context();

    // Clear the board.
    _ai_comm->Restart();

    // Then we need to randomly play the game.
    int pre_moves = _rng() % (sgf.NumMoves() / 2);
    for (int i = 0; i < pre_moves; ++i) _state.NextMove();
}

