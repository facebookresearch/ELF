/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "go_game_specific.h"
#include "sgf.h"
#include "board.h"
#include "board_feature.h"
#include <sstream>
#include <map>
#include <vector>

using namespace std;

class HandicapTable {
private:
    // handicap table.
    map<int, vector<Coord> > _handicaps;

public:
    HandicapTable();
    void Apply(int handi, Board *board) const;
};

class GoState {
public:
    GoState() : _bf(_board) { Reset(); }
    bool forward(const Coord &c);
    bool CheckMove(const Coord &c) const;

    void SetFinalValue(float final_value) {
        _final_value = final_value;
        _has_final_value = true;
    }
    float GetFinalValue() const { return _final_value; }
    bool HasFinalValue() const { return _has_final_value; }

    void Reset();
    void ApplyHandicap(int handi);

    GoState(const GoState &s) : _bf(_board) {
        CopyBoard(&_board, &s._board);
    }

    static HandicapTable &handi_table() { return _handi_table; }

    const Board &board() const { return _board; }

    bool JustStarted() const { return _board._ply == 1; }
    int GetPly() const { return _board._ply; }

    Coord LastMove() const { return _board._last_move; }
    Coord LastMove2() const { return _board._last_move2; }
    Stone NextPlayer() const { return _board._next_player; }

    vector<Coord> moves_since(int *move_number) const {
        if (*move_number < 0) {
            *move_number = _moves.size();
            return _moves;
        } else {
            vector<Coord> moves;
            for (size_t i = (size_t)*move_number; i < _moves.size(); ++i) {
                moves.push_back(_moves[i]);
            }
            *move_number = _moves.size();
            return moves;
        }
    }

    const BoardFeature &extractor(BoardFeature::Rot new_rot, bool new_flip) {
        _bf.SetD4Group(new_rot, new_flip);
        return _bf;
    }

    const BoardFeature &extractor(int code) {
        auto rot = (BoardFeature::Rot)(code % 4);
        bool flip = (code >> 2) == 1;
        _bf.SetD4Group(rot, flip);
        return _bf;
    }

    const BoardFeature &extractor() {
        _bf.SetD4Group(BoardFeature::NONE, false);
        return _bf;
    }

    const BoardFeature &last_extractor() const { return _bf; }
    const vector<Coord> &GetAllMoves() const { return _moves; }

    string ShowBoard() const {
        char buf[2000];
        ShowBoard2Buf(&_board, SHOW_LAST_MOVE, buf);
        return string(buf);
    }

protected:
    Board _board;
    BoardFeature _bf;
    vector<Coord> _moves;
    float _final_value = 0.0;
    bool _has_final_value = false;

    static HandicapTable _handi_table;
};

