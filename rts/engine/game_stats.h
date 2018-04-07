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

struct WinStats {
    int games = 0;
    int player0win = 0;
    int player1win = 0;

    void feed(PlayerId id) {
        if (id == 0) player0win ++;
        else if (id == 1) player1win ++;
        games ++;
    }

    std::string info() const {
        std::stringstream ss;
        ss << "Result:" << player0win << "/"<< player1win << "/" << games << endl;
        ss << "  Win rate (as player 0): " << (float)player0win / games << " " << player0win << "/" << games << endl;
        ss << "  Win rate (as Player 1): " << (float)player1win / games << " " << player1win << "/" << games << endl;
        return ss.str();
    }
};

struct FacilityStats {
    std::vector<int> base_choices;
    int games = 0;

    const int NUM_TOWN_HALL_CHOICES = 4;

    FacilityStats() {
        base_choices.resize(NUM_TOWN_HALL_CHOICES);
        std::fill(base_choices.begin(), base_choices.end(), 0);
    }

    void feed(int base_choice) {
        if (base_choice >= 0 && base_choice < NUM_TOWN_HALL_CHOICES) {
            base_choices[base_choice] ++;
        }
        games ++;
    }

    std::string info() const {
        std::stringstream ss;
        for (int i = 0; i < NUM_TOWN_HALL_CHOICES; i++) {
            ss << "Base loc" << i << " rate: " << (float)base_choices[i] / games << " " << base_choices[i] << "/" << games << endl;
        }
        return ss.str();
    }
};

class GlobalStats {
private:
    std::mutex _mutex;
    std::map<std::string, WinStats> _win_stats;
    FacilityStats _fac_stats;

    int _print_per_n_game;
    int _games = 0;

public:
    GlobalStats(int print_per_n_game = 0) : _print_per_n_game(print_per_n_game) {
    }

    void AccumulateWin(int base_choice, const std::string &name, PlayerId id) {
        std::unique_lock<std::mutex> lock(_mutex);
        _win_stats[name].feed(id);
        _fac_stats.feed(base_choice);

        if (_print_per_n_game > 0 && _games % _print_per_n_game == 0) {
            std::cout << PrintInfo() << std::endl;
        }
        _games ++;
    }

    std::string PrintInfo() const {
        std::stringstream ss;
        for (const auto &p : _win_stats) {
            ss << p.first << ":" << endl << p.second.info() << endl;
        }
        ss << _fac_stats.info();
        return ss.str();
    }
};

class GameStats {
private:
    int _base_choice;
    PlayerId _winner;
    std::string _winner_name;
    std::vector<float> _ratio_failed_moves;
    GlobalStats *_gstats;

    mutable std::string _last_error;

public:
    GameStats(GlobalStats *gstats = nullptr) : _gstats(nullptr) {
        Reset();
        SetGlobalStats(gstats);
    }

    void SetGlobalStats(GlobalStats *gstats) { _gstats = gstats; }

    int GetBaseChoice() const {
        return _base_choice;
    }

    bool CheckGameSmooth(Tick tick) const {
        _last_error = "";
        // Check if the game goes smoothly.
        if (_ratio_failed_moves[tick] < 1.0) return true;
        float failed_summation = 0.0;

        // Check last 30 ticks, if there is a lot of congestion, return false;
        for (int i = 0; i < min(tick, 30); ++i) {
            failed_summation += _ratio_failed_moves[tick - i];
        }
        if (failed_summation >= 250.0) {
            std::stringstream ss;
            ss << "[" << tick << "]: The game is not in good shape! sum_failed = " << failed_summation << endl;
            for (int i = 0; i < min(tick, 30); ++i) {
                ss << "  [" << tick - i << "]: " << _ratio_failed_moves[tick - i] << endl;
            }
            _last_error = ss.str();
            return false;
        } else return true;
    }

    const std::string &GetLastError() const { return _last_error; }

    // Called by the move command, record #failed moves.
    void RecordFailedMove(Tick tick, float ratio_unit_failed) {
        _ratio_failed_moves[tick] += ratio_unit_failed;
    }

    void Reset() {
        if (_gstats != nullptr) {
            _gstats->AccumulateWin(_base_choice, _winner_name, _winner);
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

    void SetWinner(const std::string &name, PlayerId id) {
        _winner = id;
        _winner_name = name;
    }
};
