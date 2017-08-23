/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

//File: atari_game.h

#pragma once

#include <vector>
#include <ale/ale_interface.hpp>
#include <string>

#include "elf/pybind_helper.h"
#include "elf/comm_template.h"
#include "elf/ai_comm.h"
#include "atari_game_specific.h"

class AtariGameSummary {
private:
    reward_t _accu_reward = 0;
    reward_t _accu_reward_all_game = 0;
    int _n_complete_game = 0;
public:
    void Feed(reward_t curr_reward);
    void Feed(const AtariGameSummary &);
    void OnEnd();
    void Print() const;
};

using Context = ContextT<GameOptions, HistT<GameState>>;
using Comm = typename Context::Comm;
using AIComm = AICommT<Comm>;

class AtariGame {
  private:
    int _game_idx = -1;
    AIComm *_ai_comm;
    std::unique_ptr<ALEInterface> _ale;

    int _width, _height;
    std::vector<Action> _action_set;

    // Used to dump the current frame.
    // h * w * 3(RGB)
    // 210 * 160 * 3
    // We also save history here.
    std::vector<unsigned char> _buf;
    CircularQueue<std::vector<float>> _h;

    float _reward_clip;
    bool _eval_only;

    float _last_reward = 0;
    AtariGameSummary _summary;

    static const int kMaxRep = 30;
    int _last_act_count = 0;
    int _last_act = -1;

    std::unique_ptr<std::uniform_int_distribution<>> _distr_action;

    int _prevent_stuck(std::default_random_engine &g, int act);
    void _reset_stuck_state();

    void _fill_state(GameState&);
    void _copy_screen(GameState &);

  public:
    AtariGame(const GameOptions&);

    void initialize_comm(int game_idx, AIComm* ai_comm) {
      assert(!_ai_comm);
      _ai_comm = ai_comm;
      _game_idx = game_idx;
    }

    void MainLoop(const std::atomic_bool& done);

    int num_actions() const { return _action_set.size(); }
    const std::vector<Action>& action_set() const { return _action_set; }
    int width() const { return _width; }
    int height() const { return _height; }
    const AtariGameSummary &summary() const { return _summary; }
};
