/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include "../../elf/comm_template.h"
#include "../../elf/pybind_interface.h"

#include "../engine/game.h"
#include "python_options.h"

using GC = ContextT<PythonOptions, ExtGame, Reply>;

class WrapperCallbacks {
private:
    int _game_idx;
    const PythonOptions &_options;
    GC::AIComm *_ai_comm;

    OmniAI *_opponent;
    OmniAI *_ai;

    float _latest_start;
    int _simple_ratio;

public:
    explicit WrapperCallbacks(int game_idx, const PythonOptions &options, GC::AIComm *ai_comm) 
        : _game_idx(game_idx), _options(options), _ai_comm(ai_comm), _opponent(nullptr), _ai(nullptr) { } 

    static void GlobalInit();
    void OnGameOptions(RTSGameOptions *rts_options);
    void OnGameInit(RTSGame *game);
    void OnEpisodeStart(int k, std::mt19937 *rng, RTSGame *game);
};
