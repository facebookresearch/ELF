#pragma once

#include "go_game_specific.h"
#include "sgf.h"
#include "board.h"
#include "board_feature.h"
#include "scoring.h"
#include "default_policy.h"
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

    void Reset();
    void ApplyHandicap(int handi);

    GoState(const GoState &s) : _bf(_board) {
        CopyBoard(&_board, &s._board);
    }

    static HandicapTable &handi_table() { return _handi_table; }

    Board &board() { return _board; }
    const Board &board() const { return _board; }

    bool JustStarted() const { return _board._ply == 1; }
    int GetPly() const { return _board._ply; }

    Coord LastMove() const { return _board._last_move; }
    Coord LastMove2() const { return _board._last_move2; }
    Stone NextPlayer() const { return _board._next_player; }

    vector<Coord> last_opponent_moves() const {
        Coord m = LastMove();
        if (m != M_PASS) return vector<Coord>{ LastMove() };
        else return vector<Coord>();
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

    string ShowBoard() const {
        char buf[2000];
        ShowBoard2Buf(&_board, SHOW_LAST_MOVE, buf);
        return string(buf);
    }

    // Use def policy and TT score to get an estimate of the value.
    float Evaluate(function<unsigned int ()> rand_func, int num_trial = 5) const {
        OwnerMap ownermap;
        DefPolicy def_policy;
        const int max_depth = BOARD_SIZE * BOARD_SIZE * 2 - _board._ply;
        for (int i = 0; i < num_trial; ++i) {
            Board board2;
            CopyBoard(&board2, &_board);
            def_policy.Run(rand_func, &board2, nullptr, max_depth, false);
            ownermap.Accumulate(&board2);
        }

        float score = ownermap.GetTTScore(&_board, nullptr, nullptr);
        // std::cout << ShowBoard() << std::endl;
        // std::cout << "Done with evaluation. score: " << score << std::endl;

        return score - 7.5 > 0.5 ? 1.0 : -1.0;
    }

protected:
    Board _board;
    BoardFeature _bf;

    static HandicapTable _handi_table;
};

