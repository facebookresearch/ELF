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
protected:
    Board _board;
    Board _last_board;
    HandicapTable _handi_table;
    BoardFeature _bf;

public:
    GoState() : _bf(_board) { Reset(); }
    bool ApplyMove(Coord c);
    void Reset();
    void ApplyHandicap(int handi);

    Board &board() { return _board; }
    Board &last_board() { return _last_board; }
    HandicapTable &handi_table() { return _handi_table; }
    bool JustStarted() const { return _board._ply == 1; }
    int GetPly() const { return _board._ply; }

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
};

