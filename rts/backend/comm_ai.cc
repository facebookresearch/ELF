/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "comm_ai.h"
#include "save2json.h"

///////////////////////////// Web TCP AI ////////////////////////////////
bool TCPAI::Act(const GameEnv &env, bool /* must_act */) {
  // First we send the visualization.
  if (_receiver->GetTick() >= _vis_after) send_vis(save_vis_data(env));

  std::string msg;
  while (queue_.try_dequeue(msg)) {
     _raw_converter.Process(env, msg, _receiver);
  }
  return true;
}

string TCPAI::save_vis_data(const GameEnv& env) const {
  bool is_spectator = (_player_id == INVALID);

  json game;
  env.FillHeader<save2json, json>(*_receiver, &game);
  env.FillIn<save2json, json>(_player_id, *_receiver, &game);

  if (is_spectator) {
    save2json::Save(*this, &game);
    // Get all current issued commands.
    save2json::SaveCmd(*_receiver, INVALID, &game);

    // Save the current replay progress.
    int replay_size = _receiver->GetLoadedReplaySize();
    if (replay_size > 0) {
      game["progress_precent"] = _receiver->GetTick() * 100 / _receiver->GetLoadedReplayLastTick();
    }
  } else {
    save2json::Save(*this, &game);
    save2json::SaveCmd(*_receiver, _player_id, &game);
  }
  return game.dump();
}

bool TCPAI::send_vis(const string &s) {
  server_->send(s);
  return true;
}
