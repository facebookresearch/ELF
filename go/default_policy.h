//
// Copyright (c) 2016-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant 
// of patent rights can be found in the PATENTS file in the same directory.
// 

#pragma once

#include "board.h"
#include <vector>
#include <functional>
using namespace std;

enum MoveType { NORMAL = 0, KO_FIGHT, OPPONENT_IN_DANGER, OUR_ATARI, NAKADE, PATTERN, NO_MOVE, NUM_MOVE_TYPE };

struct DefPolicyMove {
    Coord m;
    MoveType type;
    int gamma;
    bool game_ended;

    DefPolicyMove(Coord m, MoveType t, int gamma = 100) : m(m), type(t), gamma(gamma), game_ended(false) {
    }

    DefPolicyMove() : m(M_PASS), type(NORMAL), gamma(0), game_ended(false) {
    }
};

using RandFunc = function<unsigned int ()>;

// A queue for adding candidate moves.
class DefPolicyMoves {
public:
    const Board *board() const { return board_; }
    int size() const { return moves_.size(); }

    void clear() { moves_.clear(); total_gamma_ = 0; }

    void add(const DefPolicyMove &move) {
        moves_.push_back(move);
        total_gamma_ += move.gamma;
    }

    void remove(int i) {
        total_gamma_ -= moves_[i].gamma;
        moves_[i].gamma = 0;
    }

    const DefPolicyMove &operator[](int i) const { return moves_[i]; }
    const DefPolicyMove &at(int i) const { return moves_[i]; }

    int sample(RandFunc rand_func) const {
        if (total_gamma_ == 0) return -1;

        // Random sample.
        int stab = rand_func() % total_gamma_;
        // printf("stab = %d/%d\n", stab, total);
        for (size_t i = 0; i < moves_.size(); i++) {
            int gamma = moves_[i].gamma;
            if (stab < gamma) return i;
            stab -= gamma;
        }

        return moves_.size() - 1;
    }

    void PrintInfo(int i) const {
        char buf[30];
        ShowBoard(board_, SHOW_LAST_MOVE);
        const auto &m = moves_[i];
        util_show_move(m.m, board_->_next_player, buf);
        printf("Type = %u, gamma = %d\n", m.type, m.gamma);
    }

    DefPolicyMoves(const Board *board) 
        : board_(board) { 
    }

private:
    const Board *board_;

    // Move sequence.
    vector<DefPolicyMove> moves_;
    int total_gamma_;
};

// The parameters for default policy.
class DefPolicy {
public:
    DefPolicy();
    void PrintParams();

    DefPolicyMove Run(RandFunc, Board* board, const Region *r, int max_depth, bool verbose);

    // using CheckFunc = function<void (DefPolicy::*)(DefPolicyMoves *, const Region *)>;

private:
    bool switches_[NUM_MOVE_TYPE];
    // Try to save our group in atari if its size is >= thres_save_atari.
    int thres_save_atari_;
    // Allow self-atari move if the group size is smaller than thres_allow_atari_stone (before the new move is put).
    int thres_allow_atari_stone_;
    // Reduce opponent liberties if its liberties <= thres_opponent_libs and #stones >= thres_opponent_stones.
    int thres_opponent_libs_;
    int thres_opponent_stones_;

    // Check whether there is any ko fight we need to take part in. (Not working now)
    void check_ko_fight(DefPolicyMoves *m, const Region *r);

    // Check any of our group has lib = 1, if so, try saving it by extending.
    void check_our_atari(DefPolicyMoves *m, const Region *r);

    // Check if any opponent group is in danger, if so, try killing it.
    void check_opponent_in_danger(DefPolicyMoves *m, const Region *r);

    // Check if there is any nakade point, if so, play it to kill the opponent's group.
    void check_nakade(DefPolicyMoves *m, const Region *r);

    // Check the 3x3 pattern matcher, currently disabled.
    void check_pattern(DefPolicyMoves *m, const Region *r);

    Coord get_moves_from_group(DefPolicyMoves *m, unsigned char id, MoveType type);

    // Utilities for playing default policy. Referenced from Pachi's code.
    void compute_policy(DefPolicyMoves *m, const Region *r);

    // Sample the default policy, if ids != NULL, then only sample valid moves and save the ids information for the next play.
    bool sample(DefPolicyMoves *ms, RandFunc rand_func, bool verbose, GroupId4 *ids, DefPolicyMove *m);

    // Old version of default policy, not used.
    bool simple_sample(const DefPolicyMoves *ms, RandFunc rand_func, GroupId4 *ids, DefPolicyMove *m);
}; 

