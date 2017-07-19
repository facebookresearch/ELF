/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once
#include "../../elf/comm_template.h"
#include "../../elf/data_addr.h"
#include "../../elf/fields_common.h"

#include "python_options.h"

using GameInfo = InfoT<ExtGame, Reply>;
using Context = ContextT<PythonOptions, ExtGame, Reply>;

using DataAddr = typename Context::DataAddr;
using AIComm = typename Context::AIComm;
using Comm = typename Context::Comm;
using HistType = typename AIComm::HistType;

class FieldState : public FieldT<HistType, float> {
public:
    void ToPtr(int batch_idx, const HistType& in) override {
        const auto &info = in.newest(this->_hist_loc);
        std::copy(info.data.features.begin(), info.data.features.end(), this->addr(batch_idx));
    }
};

class FieldResource0 : public FieldT<HistType, float> {
public:
    void ToPtr(int batch_idx, const HistType& in) override {
        const auto &info = in.newest(this->_hist_loc);
        std::copy(info.data.resources[0].begin(), info.data.resources[0].end(), this->addr(batch_idx));
    }
};

class FieldResource1 : public FieldT<HistType, float> {
public:
    void ToPtr(int batch_idx, const HistType& in) override {
        const auto &info = in.newest(this->_hist_loc);
        std::copy(info.data.resources[1].begin(), info.data.resources[1].end(), this->addr(batch_idx));
    }
};

DEFINE_LAST_REWARD(HistType, float, data.last_reward);
DEFINE_REWARD(HistType, float, data.last_reward);
DEFINE_POLICY_DISTR(HistType, float, reply.action_probs);
DEFINE_LAST_TERMINAL(HistType, unsigned char);
DEFINE_TERMINAL(HistType, unsigned char);

FIELD_SIMPLE(HistType, Value, float, reply.value);
FIELD_SIMPLE(HistType, Action, int64_t, reply.global_action);

bool CustomFieldFunc(int batchsize, const std::string& key, const std::string& v, SizeType *sz, FieldBase<HistType> **p);

