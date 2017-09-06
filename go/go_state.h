#pragma once

#include "go_game_specific.h"
#include "sgf.h"
#include "board.h"
#include <sstream>
#include <map>
#include <vector>

using namespace std;

class GoState {
private:
    Sgf::SgfIterator _sgf_iter;
    Board _board;

    // handicap table.
    map<int, vector<Coord> > _handicaps;

    void build_handicap_table();

public:
    GoState() { build_handicap_table(); }
    bool NextMove();
    void Reset(const Sgf &sgf);
    void SaveTo(GameState &state, const vector<SgfMove> &future_moves) const;
    bool NeedReload() const;
    bool GetForwardMoves(vector<SgfMove> *future_moves) const;

    std::string info() const {
        std::stringstream ss;
        ss << _sgf_iter.GetCurrIdx() << "/" << _sgf_iter.GetSgf().NumMoves();
        return ss.str();
    }

    string ShowBoard() const {
        char buf[2000];
        ShowBoard2Buf(&_board, SHOW_LAST_MOVE, buf);
        return string(buf);
    }
};

