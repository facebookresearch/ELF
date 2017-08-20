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
#include <mutex>
#include "common.h"

class GlobalStats {
private:
    std::mutex _mutex;
    int _games, _player0win, _player1win, _base_choice0;
    int _print_per_n_game;

public:
    GlobalStats(int print_per_n_game = 0) : _games(0), _player0win(0), _player1win(0), _base_choice0(0), _print_per_n_game(print_per_n_game) {
    }

    void AccumulateWin(int base_choice, PlayerId id) {
        std::unique_lock<std::mutex> lock(_mutex);
        if (id == 0) _player0win ++;
        else if (id == 1) _player1win ++;
        if (base_choice == 0) _base_choice0 ++;
        _games ++;

        if (_print_per_n_game > 0 && _games % _print_per_n_game == 0) {
            std::cout << PrintInfo() << std::endl;
        }
    }

    std::string PrintInfo() const {
        std::stringstream ss;
        ss << "Result:" << _player0win << "/"<< _player1win << "/" << _games << endl;
        ss << "Player 0 win rate: " << (float)_player0win / _games << " " << _player0win << "/" << _games << endl;
        ss << "Player 1 win rate: " << (float)_player1win / _games << " " << _player1win << "/" << _games << endl;
        ss << "Base loc0 rate: " << (float)_base_choice0 / _games << " " << _base_choice0 << "/" << _games << endl;
        return ss.str();
    }
};

class GameStats {
private:
    int _base_choice;
    PlayerId _winner;
    std::vector<float> _ratio_failed_moves;
    GlobalStats *_gstats;

public:
    GameStats(GlobalStats *gstats = nullptr) : _gstats(nullptr) {
        Reset();
        SetGlobalStats(gstats);
    }

    void SetGlobalStats(GlobalStats *gstats) { _gstats = gstats; } 

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
        if (_gstats != nullptr) {
            _gstats->AccumulateWin(_base_choice, _winner);
        }
        _base_choice = -1;
        _winner = INVALID;
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

    void SetWinner(PlayerId id) {
        _winner = id;
    }
};


