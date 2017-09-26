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
bool TCPAI::on_act(Tick t, RTSMCAction *action, const std::atomic_bool *) {
  // First we send the visualization.
  if (t >= _vis_after) send_vis(save_vis_data());

  std::string msg;
  while (queue_.try_dequeue(msg)) {
     _raw_converter.Process(t, s().env(), msg, action);
  }
  return true;
}

string TCPAI::save_vis_data() const {
  bool is_spectator = (id() == INVALID);

  const CmdReceiver &recv = s().receiver();
  const GameEnv &env = s().env();

  json game;
  env.FillHeader<save2json, json>(recv, &game);
  env.FillIn<save2json, json>(id(), recv, &game);

  if (is_spectator) {
    // save2json::Save(*this, &game);
    // Get all current issued commands.
    save2json::SaveCmd(recv, INVALID, &game);

    // Save the current replay progress.
    int replay_size = recv.GetLoadedReplaySize();
    if (replay_size > 0) {
      game["progress_precent"] = recv.GetTick() * 100 / recv.GetLoadedReplayLastTick();
    }
  } else {
    // save2json::Save(*this, &game);
    save2json::SaveCmd(recv, id(), &game);
  }
  return game.dump();
}

bool TCPAI::send_vis(const string &s) {
  server_->send(s);
  return true;
}
