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

class WrapperCallbacks {
private:
    int _game_idx;
    const PythonOptions &_options;
    std::vector<Context::AIComm *> _ai_comms;

    AI *_opponent;
    AI *_ai;

    float _latest_start;
    int _simple_ratio;

public:
    explicit WrapperCallbacks(int game_idx, const PythonOptions &options, const std::vector<std::unique_ptr<Context::AIComm>> &ai_comms)
        : _game_idx(game_idx), _options(options), _opponent(nullptr), _ai(nullptr) {
          for (size_t i = 0; i < ai_comms.size(); ++i) {
              _ai_comms.push_back(ai_comms[i].get());
          }
    }

    static void GlobalInit();
    void OnGameOptions(RTSGameOptions *rts_options);
    void OnGameInit(RTSGame *game);
    void OnEpisodeStart(int k, std::mt19937 *rng, RTSGame *game);
};
