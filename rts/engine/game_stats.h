/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once
#include <vector>
#include <sstream>
#include "common.h"

class GameStats {
private:
    int _base_choice;
    std::vector<float> _ratio_failed_moves;

public:
    GameStats() {
        Reset();
    }

    int GetBaseChoice() const {
        return _base_choice;
    }

    bool CheckGameSmooth(Tick tick, std::ostream *output = nullptr) const {
        // Check if the game goes smoothly.
        if (_ratio_failed_moves[tick] < 1.0) return true;
        float failed_summation = 0.0;

        // Check last 30 ticks, if there is a lot of congestion, return false;
        for (int i = 0; i < min(tick, 30); ++i) {
            failed_summation += _ratio_failed_moves[tick - i];
        }
        if (failed_summation >= 250.0) {
            if (output) {
              *output<< "[" << tick << "]: The game is not in good shape! sum_failed = " << failed_summation << endl;
              for (int i = 0; i < min(tick, 30); ++i) {
                  *output << "  [" << tick - i << "]: " << _ratio_failed_moves[tick - i] << endl;
              }
            }
            return false;
        } else return true;
    }

    // Called by the move command, record #failed moves.
    void RecordFailedMove(Tick tick, float ratio_unit_failed) { 
        _ratio_failed_moves[tick] += ratio_unit_failed; 
    }

    void Reset() {
        _base_choice = -1;
        _ratio_failed_moves.resize(1, 0.0); 
    }

    void PickBase(int base_choice) {
        _base_choice = base_choice;
    }

    void IncTick() {
        _ratio_failed_moves.push_back(0.0); 
    }

    void SetTick(Tick tick) {
        _ratio_failed_moves.resize(tick + 1, 0);
    }
};


