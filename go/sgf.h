#pragma once

#include "common.h"
#include "board.h"
#include "../elf/tar_loader.h"
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
    // tt
    if (! ON_BOARD(x, y)) return M_PASS;
    //if (y >= 9) y --;
    return OFFSETXY(x, y);
}

inline string coord2str(Coord c) {
    int x = X(c);
    //if (x >= 8) x ++;
    int y = Y(c);
    //if (y >= 8) y ++;

    return std::string{ static_cast<char>('a' + x), static_cast<char>('a' + y) };
}

inline string coord2str2(Coord c) {
    int x = X(c);
    if (x >= 8) x ++;
    int y = Y(c);

    string s { static_cast<char>('A' + x) };
    return s + std::to_string(y + 1);
}

inline string coords2sgfstr(const vector<Coord>& moves) {
  std::string sgf = "(";
  for (size_t i = 0; i < moves.size(); i++) {
    std::string color = i % 2 == 0 ? "B" : "W";
    sgf += ";" + color + "[" + coord2str(moves[i]) + "]";
  }
  sgf += ")";
  return sgf;
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
    string win_reason;

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
};

// A library to load Sgf file for Go.
class Sgf {
private:
    // sgf, a game tree.
    SgfHeader _header;
    unique_ptr<SgfEntry> _root;
    int _num_moves;

    bool load_header(const char *s, const std::pair<int, int>& range, int *next_offset);

    static SgfEntry *load(const char *s, const std::pair<int, int>& range, int *next_offset);
    bool load_game(const string& filename, const string& game);

public:
    class iterator {
      public:
          iterator() : _curr(nullptr), _sgf(nullptr), _move_idx(-1) { }
          iterator(const Sgf &sgf) : _curr(sgf._root.get()), _sgf(&sgf), _move_idx(0) { }

          SgfMove GetCurrMove() const {
              if (done()) return SgfMove();
              else return SgfMove(_curr->player, _curr->move);
          }

          Coord GetCoord() const { if (done()) return M_PASS; else return _curr->move; }

          string GetCurrComment() const { return ! done() ? _curr->comment : string(""); }

          bool done() const { return _curr == nullptr; }

          iterator &operator ++() {
            if (! done()) {
              _curr = _curr->sibling.get();
              _move_idx ++;
            }
            return *this;
          }

          int GetCurrIdx() const { return _move_idx; }

          int StepLeft() const {
              if (_sgf == nullptr) return 0;
              return _sgf->NumMoves() - _move_idx - 1;
          }

          const Sgf &GetSgf() const { return *_sgf; }

          vector<SgfMove> GetForwardMoves(int k) const {
              auto iter = *this;
              vector<SgfMove> res;
              for (int i = 0; i < k; ++i) {
                  res.push_back(iter.GetCurrMove());
                  ++ iter;
              }
              return res;
          }

      private:
          const SgfEntry *_curr;
          const Sgf *_sgf;
          int _move_idx;
    };


    Sgf() : _num_moves(0) { }
    bool Load(const string& filename);
    bool Load(const string& gamename, elf::tar::TarLoader& tar_loader);

    iterator begin() const { return iterator(*this); }

    Stone GetWinner() const { return _header.winner; }
    int GetHandicapStones() const { return _header.handi; }
    int GetBoardSize() const { return _header.size; }
    int NumMoves() const { return _num_moves; }

    string PrintHeader() const;
    string PrintMainVariation();
};
