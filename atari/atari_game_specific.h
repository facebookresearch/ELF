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
#include "../elf/hist.h"
#include "../elf/copier.hh"

struct GameState {
    using State = GameState;
    // Seq information.
    int32_t seq;
    int32_t game_counter;
    char last_terminal;

    // This is 2x smaller images.
    std::vector<float> s;
    int32_t tick = 0;
    int32_t lives = 0;
    reward_t last_r = 0; // reward of last action

    // Reply
    int32_t a;
    float V;
    std::vector<float> pi;
    int32_t rv;

    void Clear() { a = 0; V = 0.0; fill(pi.begin(), pi.end(), 0.0); rv = 0; }

    GameState &Prepare(const SeqInfo &seq_info) {
        seq = seq_info.seq;
        game_counter = seq_info.game_counter;
        last_terminal = seq_info.last_terminal;

        Clear();
        return *this;
    }

    DECLARE_FIELD(GameState, seq, game_counter, last_terminal, s, tick, lives, last_r, a, V, pi, rv);
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

inline EntryInfo GetEntry(const std::string &entry, const std::string &key, const std::string &v) {
  auto *mm = GameState::get_mm(key);
  if (mm == nullptr) return EntryInfo();

  std::string type_name = mm->type();

  if (entry == "input") {
    if (key == "s") return EntryInfo(key, type_name, {3 * stoi(v), kHeightRatio, kWidthRatio});
    else if (key == "last_r" || key == "last_terminal") return EntryInfo(key, type_name, {1});
  } else if (entry == "reply") {
    if (key == "pi" || key == "V") return EntryInfo(key, type_name, {stoi(v)});
    else if (key == "a" || key == "rv") return EntryInfo(key, type_name, {1});
  }

  return EntryInfo();
}
