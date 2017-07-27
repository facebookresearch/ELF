/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once
#include "data_addr.h"

// Data.reward saves the reward of the last action.
// Note that newest(0) is the newest record, So:
//   1. for newest(0), the reward is in the future and we don't know (set to 0.0)
//   2. for newest(k), we can find the reward in k-1.
//
#define DEFINE_REWARD(HistType, type, last_reward) \
class FieldReward : public FieldT<HistType, type> { \
public: \
    void ToPtr(int batch_idx, const HistType& in) override { \
      auto *target = this->addr(batch_idx); \
      if (this->_hist_loc == 0) *target = type(); \
      else *target = in.newest(this->_hist_loc  - 1).last_reward; \
    } \
}

#define DEFINE_LAST_REWARD(HistType, type, last_reward) \
class FieldLastReward : public FieldT<HistType, type> { \
public: \
    void ToPtr(int batch_idx, const HistType& in) override { \
      auto *target = this->addr(batch_idx); \
      *target = in.newest(this->_hist_loc).last_reward; \
    } \
}

#define DEFINE_POLICY_DISTR(HistType, type, prob_distr) \
class FieldPolicy : public FieldT<HistType, type> { \
public: \
    void ToPtr(int batch_idx, const HistType& in) override { \
        const auto& prob = in.newest(this->_hist_loc).prob_distr; \
        auto *target = this->addr(batch_idx); \
        if (prob.size() == (size_t)this->_stride) std::copy(prob.begin(), prob.end(), target); \
        else std::fill(target, target + this->_stride, 0); \
    } \
    void FromPtr(int batch_idx, HistType& in) const override { \
        auto &prob = in.newest(this->_hist_loc).prob_distr; \
        const auto *target = this->addr(batch_idx); \
        if (prob.size() != (size_t)_stride) prob.resize(this->_stride); \
        std::copy(target, target + this->_stride, prob.begin()); \
    } \
}

#define DEFINE_LAST_TERMINAL(HistType, type) \
class FieldLastTerminal : public FieldT<HistType, type> { \
public: \
    void ToPtr(int batch_idx, const HistType& in) override { \
        const auto &record = in.newest(this->_hist_loc); \
        *this->addr(batch_idx) = (record.GetSeq() == 0 && record.GetGameCounter() > 0) ? 1 : 0; \
    } \
}

#define DEFINE_TERMINAL(HistType, type) \
class FieldTerminal : public FieldT<HistType, type> { \
public: \
    void ToPtr(int batch_idx, const HistType& in) override { \
      auto *target = this->addr(batch_idx); \
      if (this->_hist_loc == 0) *target = 0; \
      else { \
        const auto &record = in.newest(this->_hist_loc - 1); \
        *target = (record.GetSeq() == 0 && record.GetGameCounter() > 0) ? 1: 0; \
      } \
    } \
}

