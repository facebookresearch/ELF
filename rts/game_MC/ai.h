/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include "engine/cmd_util.h"
#include "engine/cmd_interface.h"
#include "engine/game_state.h"
#include "elf/ai.h"
#include "elf/mcts.h"
#include "game_action.h"
#include "python_options.h"
#define NUM_RES_SLOT 5

using Comm = Context::Comm;
using AIComm = AICommT<Comm>;
using Data = typename AIComm::Data;

using AIWithComm = elf::AIWithCommT<RTSState, RTSMCAction, AIComm>;
using AI = elf::AI_T<RTSState, RTSMCAction>;

struct ReducedState {
    vector<float> state;
    int action;
};

struct ReducedPred : public mcts::NodeResponseT<int> {
    void SetPiAndV(const vector<float>& new_pi, float new_v) {
        this->pi.resize(new_pi.size());
        for (size_t i = 0; i < new_pi.size(); ++i) {
            this->pi[i].first = i;
            this->pi[i].second = new_pi[i];
        }
        this->value = new_v;
    }
};
