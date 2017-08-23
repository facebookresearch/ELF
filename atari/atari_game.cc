/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

//File: atari_game.cc


#include "atari_game.h"
#include <mutex>
#include <cmath>
#include <iostream>
#include <chrono>

using namespace std;
using namespace std::chrono;

namespace {
// work around bug in ALE.
// see Arcade-Learning-Environment/issues/86
mutex ALE_GLOBAL_LOCK;
}

static long compute_seed(int v) {
  auto now = system_clock::now();
  auto now_ms = time_point_cast<milliseconds>(now);
  auto value = now_ms.time_since_epoch();
  long duration = value.count();
  long seed = (time(NULL) * 1000 + duration) % 100000000 + v;
  return seed;
}

void AtariGameSummary::Feed(reward_t last_reward) {
  _accu_reward += last_reward;
}

void AtariGameSummary::Feed(const AtariGameSummary &other) {
  _accu_reward_all_game += other._accu_reward_all_game;
  _n_complete_game += other._n_complete_game;
}

void AtariGameSummary::OnEnd() {
  _accu_reward_all_game += _accu_reward;
  _accu_reward = 0;
  _n_complete_game ++;
}

void AtariGameSummary::Print() const {
    if (_n_complete_game > 0) {
       std::cout << (float)_accu_reward_all_game / _n_complete_game << "[" << _n_complete_game << "]";
    } else {
       std::cout << "0[0]";
    }
    std::cout << " current accumulated reward: " << _accu_reward << std::endl;
}

AtariGame::AtariGame(const GameOptions& opt)
  : _h(opt.hist_len), _reward_clip(opt.reward_clip), _eval_only(opt.eval_only) {
  lock_guard<mutex> lg(ALE_GLOBAL_LOCK);
  _ale.reset(new ALEInterface);
  long seed = compute_seed(opt.seed);
  // std::cout << "Seed: " << seed << std::endl;
  _ale->setInt("random_seed", seed);
  _ale->setBool("showinfo", false);
  // _ale->setInt("frame_skip", opt.frame_skip);
  _ale->setBool("color_averaging", false);
  _ale->setFloat("repeat_action_probability", opt.repeat_action_probability);
  _ale->loadROM(opt.rom_file);

  auto& s = _ale->getScreen();
  _width = s.width(), _height = s.height();
  _action_set = _ale->getMinimalActionSet();
  _distr_action.reset(new std::uniform_int_distribution<>(0, _action_set.size() - 1));
  _buf.resize(kBufSize, 0);
}

void AtariGame::MainLoop(const std::atomic_bool& done) {
  assert(_game_idx >= 0);  // init_comm has been called

  // Add some random actions.
  long seed = compute_seed(_game_idx);
  std::default_random_engine g;
  g.seed(seed);
  std::uniform_int_distribution<int> distr_start_loc(0, 0);
  std::uniform_int_distribution<int> distr_frame_skip(2, 4);

  while (true) {
    _ale->reset_game();
    _last_reward = 0;
    int start_loc = distr_start_loc(g);

    for (int i = 0;;i ++) {
      if (done.load()) {
        return;
      }
      auto& gs = _ai_comm->Prepare();
      _fill_state(gs);

      int act;
      if (i < start_loc) {
          act = (*_distr_action)(g);
          gs.a = act;
          gs.V = 0;
      } else {
          _ai_comm->SendDataWaitReply();
          act = gs.a;
          // act = (*_distr_action)(g);
          // std::cout << "[" << _game_idx << "]: " << act << std::endl;
      }

      // Illegal action.
      // std::cout << "[" << _game_idx << "][" << gs.seq.game_counter << "][" << gs.seq.seq << "] act: "
      //          << act << "[a=" << gs.reply.action << "][V=" << gs.reply.value << "]" << std::endl;
      if (act < 0 || act >= (int)_action_set.size() || _ale->game_over()) break;
      if (_eval_only) {
          act = _prevent_stuck(g, act);
          gs.a = act;
      }
      int frame_skip = distr_frame_skip(g);
      _last_reward = 0;
      for (int j = 0; j < frame_skip; ++j) {
          _last_reward += _ale->act(_action_set.at(act));
      }
      _summary.Feed(_last_reward);
    }
    _ai_comm->Restart();
    _reset_stuck_state();
    _summary.OnEnd();
  }
}

static void _downsample(const std::vector<unsigned char> &buf, float *output) {
    for (int k = 0; k < 3; ++k) {
        const unsigned char *raw_img = &buf[k];
        for (int j = 0; j < kHeight / kRatio; ++j) {
            for (int i = 0; i < kWidth / kRatio; ++i) {
                float v = 0.0;
                int ii = kRatio * i;
                int jj = kRatio * j;
                /*
                for (int lx = 0; lx < ratio; ++lx) {
                    for (int ly = 0; ly < ratio; ++ly) {
                        v += *(raw_img + ((jj + ly) * width + (ii + lx)) * 3);
                    }
                }
                v /= (ratio * ratio);
                */
                v = *(raw_img + (jj * kWidth + ii) * 3);
                *output ++ = v / 255.0;
            }
        }
    }
}

int AtariGame::_prevent_stuck(std::default_random_engine &g, int act) {
  if (act == _last_act) {
    _last_act_count ++;
    if (_last_act_count >= kMaxRep) {
      // The player might get stuck. Save it.
      act = (*_distr_action)(g);
    }
  } else {
    // Reset counter.
    _last_act = act;
    _last_act_count = 0;
  }
  return act;
}

void AtariGame::_reset_stuck_state() {
  _last_act_count = 0;
  _last_act = -1;
}

void AtariGame::_copy_screen(GameState &state) {
    _ale->getScreenRGB(_buf);
    if (_h.full()) _h.Pop();

    const int stride = kInputStride;
    // Then copy it to the current state.
    // Stride hard coded.
    auto &item = _h.ItemPush();
    item.resize(stride);

    // Downsample the image.
    _downsample(_buf, &item[0]);
    _h.Push();

    // Then you put all the history state to game state.
    state.s.resize(_h.maxlen() * stride);
    for (int i = 0; i < _h.size(); ++i) {
        const auto &v = _h.get_from_push(i);
        std::copy(v.begin(), v.end(), &state.s[i * stride]);
    }
    if (_h.size() < _h.maxlen()) {
        const int n_missing = _h.maxlen() - _h.size();
        ::memset(&state.s[_h.size() * stride], 0, sizeof(float) * sizeof(n_missing * stride));
    }
}

void AtariGame::_fill_state(GameState& state) {
    state.tick = _ale->getEpisodeFrameNumber();
    state.last_r = _last_reward;
    if (_reward_clip > 0.0) {
        state.last_r = std::max(std::min(state.last_r, _reward_clip), -_reward_clip);
    }
    state.lives = _ale->lives();
    _copy_screen(state);
}


