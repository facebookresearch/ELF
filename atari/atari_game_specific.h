/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include <ale/ale_interface.hpp>

#include "../elf/pybind_helper.h"
#include "../elf/comm_template.h"
#include "../elf/fields_common.h"

// use int rather than Action enum as reply
struct Reply {
  int action;
  float value;
  std::vector<float> prob;
  Reply(int action = 0, float value = 0.0) : action(action), value(value) { }
  void Clear() { action = 0; value = 0.0; fill(prob.begin(), prob.end(), 0.0); }
};

struct GameState {
    // This is 2x smaller images.
    std::vector<float> buf;
    int tick = 0;
    int lives = 0;
    reward_t last_reward = 0; // reward of last action
};

struct GameOptions {
  std::string rom_file;
  int frame_skip = 1;
  float repeat_action_probability = 0.;
  int seed = 0;
  int hist_len = 4;
  reward_t reward_clip = 1.0;
  REGISTER_PYBIND_FIELDS(rom_file, frame_skip, repeat_action_probability, seed, hist_len, reward_clip);
};

using GameInfo = InfoT<GameState, Reply>;
using Context = ContextT<GameOptions, GameState, Reply>;

using DataAddr = typename Context::DataAddr;
using AIComm = typename Context::AIComm;
using Comm = typename Context::Comm;

class FieldState : public FieldT<AIComm, float> {
public:
    void ToPtr(int batch_idx, const AIComm& ai_comm) override {
        const auto &info = ai_comm.newest(this->_hist_loc);
        std::copy(info.data.buf.begin(), info.data.buf.end(), this->addr(batch_idx));
    }
};

DEFINE_LAST_REWARD(AIComm, float, data.last_reward);
DEFINE_REWARD(AIComm, float, data.last_reward);
DEFINE_POLICY_DISTR(AIComm, float, reply.prob);

DEFINE_TERMINAL(AIComm, unsigned char);
DEFINE_LAST_TERMINAL(AIComm, unsigned char);

FIELD_SIMPLE(AIComm, Value, float, reply.value);
FIELD_SIMPLE(AIComm, Action, int64_t, reply.action);

using DataAddr = DataAddrT<AIComm>;
using DataAddrService = DataAddrServiceT<AIComm>;

static constexpr int kWidth = 160;
static constexpr int kHeight = 210;
static constexpr int kRatio = 2;
static constexpr int kInputStride = kWidth*kHeight*3/kRatio/kRatio;
static constexpr int kWidthRatio = kWidth / kRatio;
static constexpr int kHeightRatio = kHeight / kRatio;

bool CustomFieldFunc(int batchsize, const std::string& key, const std::string& v, SizeType *sz, FieldBase<AIComm> **p);
