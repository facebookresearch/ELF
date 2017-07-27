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
#include "../elf/copier.h"

// use int rather than Action enum as reply
struct Reply {
  int a;
  float V;
  std::vector<float> pi;
  Reply(int action = 0, float value = 0.0) : a(action), V(value) { }
  void Clear() { a = 0; V = 0.0; fill(pi.begin(), pi.end(), 0.0); }

  DECLARE_FIELD(Reply, a, V, pi);
};

struct GameState {
  // This is 2x smaller images.
  std::vector<float> s;
  int tick = 0;
  int lives = 0;
  reward_t last_r = 0; // reward of last action

  DECLARE_FIELD(GameState, s, tick, lives, last_r);
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

static constexpr int kWidth = 160;
static constexpr int kHeight = 210;
static constexpr int kRatio = 2;
static constexpr int kInputStride = kWidth*kHeight*3/kRatio/kRatio;
static constexpr int kWidthRatio = kWidth / kRatio;
static constexpr int kHeightRatio = kHeight / kRatio;

inline elf::SharedBufferSize GetInputEntry(const std::string &key, const std::string &v) {
  if (key == "s") return elf::SharedBufferSize{3 * stoi(v), kHeightRatio, kWidthRatio};
  else if (key == "r" || key == "last_r" || key == "terminal" || key == "last_terminal") return elf::SharedBufferSize{1};
  else return elf::SharedBufferSize{0};
}

inline elf::SharedBufferSize GetReplyEntry(const std::string &key, const std::string &v) {
  if (key == "pi" || key == "V") return elf::SharedBufferSize{stoi(v)};
  else if (key == "a") elf::SharedBufferSize{1};
  else return elf::SharedBufferSize{0};
}
