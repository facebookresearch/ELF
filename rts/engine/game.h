/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include <algorithm>
#include <string>
#include <iostream>
#include <queue>
#include <set>
#include "game_options.h"
#include "game_state.h"
#include "elf/ai.h"
#include "elf/utils.h"

using namespace std;

// A tick-based RTS game.
class RTSStateExtend : public RTSState {
public:
    // Initialize the game.
    explicit RTSStateExtend(const RTSGameOptions &options);
    ~RTSStateExtend();

    // Function used in GameLoop
    bool Init(bool isPrint) override;
    void PreAct() override;
    void IncTick() override;

    bool forward(RTSAction &a) {
        return RTSState::forward(a);
    }

    bool forward(ReplayLoader::Action &actions) override {
        if (! RTSState::forward(actions)) return false;

        // Then we also need to send UI commands, if there is any.
        for (const auto &cmd : actions.ui_cmds) {
            dispatch_cmds(cmd);
        }
        return true;
    }

    elf::GameResult PostAct() override;

private:
    // Options.
    RTSGameOptions _options;

    // Next snapshot to load.
    int _snapshot_to_load;

    bool _paused;
    bool _tick_prompt;
    bool _tick_verbose;

    chrono::time_point<std::chrono::system_clock> _time_loop_start;

    elf_utils::MyClock _clock;

    string _prefix;

    // Output file to save.
    bool _output_stream_owned;
    ostream *_output_stream;

    // Dispatch commands received from gui.
    CmdReturn dispatch_cmds(const UICmd& cmd);

    bool change_simulation_speed(float fraction);
};
