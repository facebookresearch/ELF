/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include <algorithm>
#include <string>
#include <iostream>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#define sleep(n)    Sleep(n)
#endif

#include <queue>
#include <set>
#include "game_options.h"
#include "game_state.h"
#include "ai.h"
#include "elf/ai.h"

// A tick-based RTS game.
class RTSStateExtend {
public:
    // Initialize the game.
    explicit RTSStateExtend(const RTSGameOptions &options);
    ~RTSStateExtend();

    // Function used in GameLoop
    void Init();
    void PreAct();
    void IncTick();

    elf::GameResult PostAct();
    void Forward(const RTSAction &a) { _state.Forward(a); }
    void Finalize() { _state.Finalize(); }

    void OnAddPlayer(int player_id) { _state.OnAddPlayer(player_id); }
    void OnRemovePlayer(int player_id) { _state.OnRemovePlayer(player_id); }

private:
    // Options.
    RTSGameOptions _options;

    RTSState _state; 

    // Next snapshot to load.
    int _snapshot_to_load;

    bool _paused;

    // Output file to save.
    bool _output_stream_owned;
    ostream *_output_stream;

    // Dispatch commands received from gui.
    CmdReturn dispatch_cmds(const UICmd& cmd);

    bool RTSGame::change_simulation_speed(float fraction) {
        _options.main_loop_quota /= fraction;
        return true;
    }
};

#endif
