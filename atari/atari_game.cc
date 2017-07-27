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

AtariGame::AtariGame(const GameOptions& opt) : _h(opt.hist_len) {
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
  _reward_clip = opt.reward_clip;
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
      _ai_comm->Prepare();
      _fill_state(_ai_comm->info().data.newest().state);

      int act;
      if (i < start_loc) {
          act = (*_distr_action)(g);
          _ai_comm->info().data.newest().reply = Reply(act, 0.0);
      } else {
          _ai_comm->SendDataWaitReply();
          act = _ai_comm->info().data.newest().reply.action;
          // act = (*_distr_action)(g);
          // std::cout << "[" << _game_idx << "]: " << act << std::endl;
      }

      // Illegal action.
      if (act < 0 || act >= _action_set.size() || _ale->game_over()) break;
      act = _prevent_stuck(g, act);
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
      _ai_comm->info().data.newest().reply.action = act;
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

void AtariGame::_copy_screen(State &state) {
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
    state.buf.resize(_h.maxlen() * stride);
    for (int i = 0; i < _h.size(); ++i) {
        const auto &v = _h.get_from_push(i);
        std::copy(v.begin(), v.end(), &state.buf[i * stride]);
    }
    if (_h.size() < _h.maxlen()) {
        const int n_missing = _h.maxlen() - _h.size();
        ::memset(&state.buf[_h.size() * stride], 0, sizeof(float) * sizeof(n_missing * stride));
    }
}

void AtariGame::_fill_state(State& state) {
    state.tick = _ale->getEpisodeFrameNumber();
    state.last_reward = _last_reward;
    if (_reward_clip > 0.0) {
        state.last_reward = std::max(std::min(state.last_reward, _reward_clip), -_reward_clip);
    }
    state.lives = _ale->lives();
    _copy_screen(state);
}

bool CustomFieldFunc(int batchsize, const std::string& key,
    const std::string& v, SizeType *sz, FieldBase<GameInfo> **p) {
    // Note that ptr and stride will be set after the memory are initialized in the Python side.
    if (key == "s") {
        const int hist_len = stoi(v);
        *sz = SizeType{batchsize, 3 * hist_len, kHeightRatio, kWidthRatio};
        *p = new FieldState();
    } else if (key == "pi") {
        const int action_len = stoi(v);
        *sz = SizeType{batchsize, action_len};
        *p = new FieldPolicy();
    } else if (key == "a") {
        *sz = SizeType{batchsize};
        *p = new FieldAction();
    } else if (key == "r") {
        // const float reward_limit = stof(v);
        *sz = SizeType{batchsize};
        *p = new FieldReward();
    } else if (key == "last_r") {
        // const float reward_limit = stof(v);
        *sz = SizeType{batchsize};
        *p = new FieldLastReward();
    } else if (key == "V") {
        const int value_len = stoi(v);
        *sz = SizeType{batchsize, value_len};
        *p = new FieldValue();
    } else if (key == "terminal") {
        *sz = SizeType{batchsize};
        *p = new FieldTerminal();
    } else if (key == "last_terminal") {
        *sz = SizeType{batchsize};
        *p = new FieldLastTerminal();
    } else {
        return false;
    }
    return true;
}
