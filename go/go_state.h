#pragma once

#include "go_game_specific.h"
#include "sgf.h"
#include "board.h"
#include "board_feature.h"
#include <sstream>
#include <map>
#include <vector>

using namespace std;

#define BOARD_DIM 19
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

    void Reset();
    void ApplyHandicap(int handi);

    GoState(const GoState &s) : _bf(_board) {
        CopyBoard(&_board, &s._board); 
    }

    static HandicapTable &handi_table() { return _handi_table; }

    Board &board() { return _board; }
    bool JustStarted() const { return _board._ply == 1; }
    int GetPly() const { return _board._ply; }

    Coord LastMove() const { return _board._last_move; }
    Coord LastMove2() const { return _board._last_move2; }
    Stone NextPlayer() const { return _board._next_player; }

    const BoardFeature &extractor(BoardFeature::Rot new_rot, bool new_flip) {
        _bf.SetD4Group(new_rot, new_flip);
        return _bf;
    }

    const BoardFeature &extractor() {
        _bf.SetD4Group(BoardFeature::NONE, false);
        return _bf;
    }

    const BoardFeature &last_extractor() const { return _bf; }

    string ShowBoard() const {
        char buf[2000];
        ShowBoard2Buf(&_board, SHOW_LAST_MOVE, buf);
        return string(buf);
    }

protected:
    Board _board;
    BoardFeature _bf;

    static HandicapTable _handi_table;
};

