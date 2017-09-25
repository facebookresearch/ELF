/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "go_loader.h"

/////////////////////////////////////////////////
void Loader::Act(const std::atomic_bool& done) {
  if (! Ready(done)) return;
  if (state().JustStarted()) _ai_comm->Restart();

  // Send the current board situation.
  auto& gs = _ai_comm->Prepare();

  SaveTo(gs);

  // There is always only 1 player.
  _ai_comm->SendDataWaitReply();

  Next(_ai_comm->info().data.newest().a);
}

void Loader::UndoMove() {
    _state.board() = _state.last_board();
}

void Loader::ApplyHandicap(int handi) {
    _state.last_board() = _state.board();
    _state.ApplyHandicap(handi);
}
