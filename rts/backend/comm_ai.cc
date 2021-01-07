/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "comm_ai.h"

void WebCtrl::Extract(const RTSState &s, json *game) {
    PlayerId id = _raw_converter.GetPlayerId();

    const CmdReceiver &recv = s.receiver();
    const GameEnv &env = s.env();

    env.FillHeader<save2json, json>(recv, game);
    env.FillIn<save2json, json>(id, recv, game);
    save2json::SaveCmd(recv, id, game);

    for (const int &i : _raw_converter.GetAllSelectedUnits()) {
        (*game)["selected_units"].push_back(i);
    }
}

void WebCtrl::Receive(const RTSState &s, vector<CmdBPtr> *cmds, vector<UICmd> *ui_cmds) {
    std::string msg;
    while (queue_.try_dequeue(msg)) {
        _raw_converter.Process(s.GetTick(), s.env(), msg, cmds, ui_cmds);
    }
}

///////////////////////////// Web TCP AI ////////////////////////////////
bool TCPAI::Act(const State &s, RTSMCAction *action, const std::atomic_bool *) {
    // First we send the visualization.
    //std::cout<<"=======TCPAI Act======="<<std::endl;
    json game;
    _ctrl.Extract(s, &game);
    _ctrl.Send(game.dump());

    vector<CmdBPtr> cmds;
    vector<UICmd> ui_cmds;

    _ctrl.Receive(s, &cmds, &ui_cmds);
    // Put the cmds to action, ignore all ui cmds.
    // [TODO]: Move this to elf::game_base.h. 
    action->Init(id(), name()); 
    for (auto &&cmd : cmds) {
        action->cmds().emplace(make_pair(cmd->id(), std::move(cmd)));
    }
    return true;
}

bool TCPSpectator::Act(const RTSState &s, Action *action, const std::atomic_bool *) {
    Tick tick = s.GetTick();

    while (tick >= (int)_history_states.size()) {
        _history_states.emplace_back();
    }
    if (_history_states[tick].empty()) {
        s.Save(&_history_states[tick]);
    } else {
        s.Save(&_history_states[tick]);
        /*
        string new_state;
        s.Save(&new_state);
        if (new_state.compare(_history_states[tick]) != 0) {
            cout << "[" << tick << "] New saved state is different from old one" << endl;
            s.SaveSnapshot("new_" + std::to_string(tick) + ".txt", false);
            RTSState old_s;
            old_s.Load(_history_states[tick]);
            old_s.SaveSnapshot("old_" + std::to_string(tick) + ".txt", false);
            
            throw std::range_error("Error!");
        }
        */
    }

    if (tick >= _vis_after) {
        json game;
        _ctrl.Extract(s, &game);

        int replay_size = GetLoadedReplaySize();
        if (replay_size > 0) {
            game["replay_length"] = GetLoadedReplayLastTick();
        }

        _ctrl.Send(game.dump());
    }

    // action->Init(_ctrl.GetId(), "spectator"); 
    _ctrl.Receive(s, &action->cmds, &action->ui_cmds);
    for (const UICmd &cmd : action->ui_cmds) {
        switch (cmd.cmd) {
            case UI_CYCLEPLAYER:
                {
                    PlayerId id = _ctrl.GetId(); 
                    if (id == INVALID) id = 0;
                    else id ++;
                    if (id >= s.env().GetNumOfPlayers()) id = INVALID;
                    _ctrl.SetId(id);
                }
                break;
            case UI_SLIDEBAR:
                {
                    // UI controls, only works if there is a spectator.
                    // cout << "Receive slider bar notification " << cmd.arg2 << endl;
                    float r = cmd.arg2 / 100.0;
                    Tick new_tick = static_cast<Tick>(GetLoadedReplayLastTick() * r);
                    if (new_tick < (int)_history_states.size()) {
                        // cout << "Switch back from tick = " << tick << " to new tick = " << new_tick << endl;
                        tick = new_tick;
                        action->new_state = _history_states[tick];
                        ReplayLoader::Relocate(tick);
                    }
                }
                break;
            default:
                break;
        }
    }

    ReplayLoader::SendReplay(tick, action);
    return true;
}

