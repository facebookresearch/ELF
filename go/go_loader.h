/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include <map>
#include <vector>
#include "go_state.h"

using namespace std;

class Loader {
protected:
    GoState _state;
    AIComm *_ai_comm = nullptr;

public:
    Loader(AIComm *ai_comm) : _ai_comm(ai_comm) { }
    
    virtual bool Ready(const std::atomic_bool &done) { (void)done; return true; }
    virtual void SaveTo(GameState &state) = 0;
    virtual void Next(int64_t action) = 0;
    
    virtual void Act(const std::atomic_bool& done);

    void ApplyHandicap(int handi);
    void UndoMove();

    const GoState &state() const { return _state; }
};

