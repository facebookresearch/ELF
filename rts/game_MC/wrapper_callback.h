/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include "elf/comm_template.h"
#include "elf/pybind_interface.h"

#include "engine/game.h"
#include "engine/wrapper_template.h"
#include "python_options.h"

class WrapperCallbacks {
private:
    int _game_idx;
    const ContextOptions &_context_options;
    const PythonOptions &_options;

    Context::Comm *_comm;

    std::vector<std::unique_ptr<Context::AIComm>> _ai_comms;

    void initialize_ai_comm(Context::AIComm &ai_comm);

public:
    explicit WrapperCallbacks(int game_idx, const ContextOptions &context_options, const PythonOptions &options, Context::Comm *comm)
        : _game_idx(game_idx), _context_options(context_options), _options(options), _comm(comm) {
    }

    void OnGameOptions(RTSGameOptions *rts_options);
    void OnGameInit(RTSGame *game);
    void OnEpisodeStart(int k, std::mt19937 *rng, RTSGame *game);
};
