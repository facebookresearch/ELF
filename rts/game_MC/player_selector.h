/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "ai.h"

class PlayerSelector {
public:
    static AI* GetPlayer(std::string player, int frame_skip) {
        if (player == "simple") return new SimpleAI(INVALID, frame_skip, nullptr);
        else if (player == "hit_and_run") return new HitAndRunAI(INVALID, frame_skip, nullptr);
        else {
            cout << "Unknown player " << player << endl;
            return nullptr;
        }
    }
};
