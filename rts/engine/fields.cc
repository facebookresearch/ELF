/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "fields.h"

bool CustomFieldFunc(int batchsize, const std::string& key, const std::string& v, SizeType *sz, FieldBase<AIComm> **p) {
    // Note that ptr and stride will be set after the memory are initialized in the Python side.
    if (key == "s") {
        const int channel_size = stoi(v);
        *sz = SizeType{batchsize, channel_size, 20, 20};
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
    } else if (key == "r0") {
        *sz = SizeType{batchsize, 5};
        *p = new FieldResource0();
    } else if (key == "r1") {
        *sz = SizeType{batchsize, 5};
        *p = new FieldResource1();
    } else {
        return false;
    }
    return true;
}
