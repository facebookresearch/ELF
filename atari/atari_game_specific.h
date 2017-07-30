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

static constexpr int kWidth = 160;
static constexpr int kHeight = 210;
static constexpr int kRatio = 2;
static constexpr int kInputStride = kWidth*kHeight*3/kRatio/kRatio;
static constexpr int kWidthRatio = kWidth / kRatio;
static constexpr int kHeightRatio = kHeight / kRatio;

struct GameState {
    using State = GameState;
    // Seq information.
    int32_t id = -1;
    int32_t seq = 0;
    int32_t game_counter = 0;
    char last_terminal = 0;

    // This is 2x smaller images.
    std::vector<float> s;
    int32_t tick = 0;
    int32_t lives = 0;
    float last_r = 0.0; // reward of last action

    // Reply
    int64_t a;
    float V;
    std::vector<float> pi;
    int32_t rv;

    void Clear() { a = 0; V = 0.0; fill(pi.begin(), pi.end(), 0.0); rv = 0; }

    void Init(int iid, int num_action) {
        id = iid;
        pi.resize(num_action, 0.0);
    }

    GameState &Prepare(const SeqInfo &seq_info) {
        seq = seq_info.seq;
        game_counter = seq_info.game_counter;
        last_terminal = seq_info.last_terminal;

        Clear();
        return *this;
    }

    std::string PrintInfo() const {
        std::stringstream ss;
        ss << "[id:" << id << "][seq:" << seq << "][game_counter:" << game_counter << "][last_terminal:" << last_terminal << "]";
        return ss.str();
    }

    void Restart() {
        tick = 0;
        lives = 0;
        last_r = 0; // reward of last action
        seq = 0;
        game_counter = 0;
        last_terminal = 0;
    }

    DECLARE_FIELD(GameState, id, seq, game_counter, last_terminal, s, tick, lives, last_r, a, V, pi, rv);
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

