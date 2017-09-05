#ifndef _SGF_H_
#define _SGF_H_

#include "common.h"
#include "board.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>

using namespace std;

// Load the remaining part.
inline Coord str2coord(const string &s) {
    if (s.size() < 2) return M_PASS;
    int x = s[0] - 'a';
    //if (x >= 9) x --;
    int y = s[1] - 'a';
    //if (y >= 9) y --;
    return OFFSETXY(x, y);
}

inline string coord2str(Coord c) {
    int x = X(c);
    //if (x >= 8) x ++;
    int y = Y(c);
    //if (y >= 8) y ++;

    string s;
    s.resize(3);
    s[0] = 'a' + x;
    s[1] = 'a' + y;
    s[2] = 0;
    return s;
}

struct SgfEntry {
    Coord move;
    Stone player;
    string comment;

    // All other (key, value) pairs.
    map<string, string> kv;

    // Tree structure.
    unique_ptr<SgfEntry> child;
    unique_ptr<SgfEntry> sibling;
};

struct SgfHeader {
    int rule;
    int size;
    float komi;
    int handi;
    string white_name, black_name;
    string white_rank, black_rank;
    string comment;

    Stone winner;
    float win_margin;

    void Reset() {
        rule = 0;
        size = 19;
        komi = 7.5;
        handi = 0;

        white_name = "";
        black_name = "";
        white_rank = "";
        black_rank = "";

        winner = S_OFF_BOARD;
        win_margin = 0.0;
    }
};

struct SgfMove {
    Stone player;
    Coord move;

    SgfMove() : player(S_OFF_BOARD), move(M_INVALID) {
    }

    SgfMove(Stone p, Coord m) : player(p), move(m) {
    }

    int GetAction() const {
        return EXPORT_OFFSET_XY(X(move), Y(move));
    }
};

// A library to load Sgf file for Go.
class Sgf {
private:
    // sgf, a game tree.
    SgfHeader _header;
    unique_ptr<SgfEntry> _root;
    const SgfEntry *_curr;
    int _move_idx;
    int _num_moves;

    bool load_header(const char *s, const std::pair<int, int>& range, int *next_offset);

    static SgfMove get_move(const SgfEntry *curr) {
        if (curr == nullptr) return SgfMove();
        else return SgfMove(curr->player, curr->move);
    }
    static const SgfEntry *get_next(const SgfEntry *curr) {
        if (curr == nullptr) return nullptr;
        else return curr->sibling.get();
    }
    static SgfEntry *load(const char *s, const std::pair<int, int>& range, int *next_offset);

public:
    Sgf() : _curr(nullptr), _move_idx(-1), _num_moves(0) { }
    bool Load(const string& filename);
    SgfMove GetCurr() const { return get_move(_curr); }
    int GetCurrMoveIdx() const { return _move_idx; }

    vector<SgfMove> GetForwardMoves(int k) const {
        const SgfEntry *curr = _curr;
        vector<SgfMove> res;
        for (int i = 0; i < k; ++i) {
            res.push_back(get_move(curr));
            curr = get_next(curr);
        }
        return res;
    }

    string GetCurrComment() const { return _curr != nullptr ? _curr->comment : string(""); }
    bool Next();
    void Reset();

    Stone GetWinner() const { return _header.winner; }
    int GetHandicapStones() const { return _header.handi; }
    int StepLeft() const { return _num_moves - _move_idx - 1; }
    int NumMoves() const { return _num_moves; }

    string PrintHeader() const;
    string PrintMainVariation();
};

#endif
