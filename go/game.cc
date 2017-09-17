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
GoGame::GoGame(int game_idx, const GameOptions& options) : _options(options), _curr_loader_idx(0) {
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

    if (_options.online) {
        _loaders.emplace_back(new OnlinePlayer());
    } else {
        // Open many offline instances.
        for (int i = 0; i < _options.num_games_per_thread; ++i) {
            _loaders.emplace_back(new OfflineLoader(_options, seed + _game_idx * i * 997 + i * 13773 + 7));
        }
    }
    if (_options.verbose) std::cout << "[" << _game_idx << "] Done with initialization" << std::endl;
}

void GoGame::Act(const std::atomic_bool& done) {
  // Randomly pick one loader
  _curr_loader_idx = _rng() % _loaders.size();
  Loader *loader = _loaders[_curr_loader_idx].get();

  if (! loader->Ready(done)) return;
  if (loader->state().JustStarted()) _ai_comm->Restart();

  // Send the current board situation.
  auto& gs = _ai_comm->Prepare();

  loader->SaveTo(gs);

  // There is always only 1 player.
  _ai_comm->SendDataWaitReply();

  loader->Next(_ai_comm->info().data.newest().a);
}
