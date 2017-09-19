#pragma once

#include "go_game_specific.h"
#include "sgf.h"
#include "board.h"
#include "board_feature.h"
#include "elf/shared_replay_buffer.h"
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

class Loader {
protected:
    GoState _state;

public:
    virtual bool Ready(const std::atomic_bool &done) { (void)done; return true; }
    virtual void SaveTo(GameState &state) = 0;
    virtual void Next(int64_t action) = 0;
    void ApplyHandicap(int handi);
    void UndoMove();

    const GoState &state() const { return _state; }
};

using RBuffer = SharedReplayBuffer<std::string, Sgf>;

class OfflineLoader : public Loader {
protected:
    // Shared buffer for OfflineLoader.
    static std::unique_ptr<RBuffer> _rbuffer;
    static std::unique_ptr<TarLoader> _tar_loader;

    Sgf::SgfIterator _sgf_iter;
    string _list_filename;
    string _path;

    // Database
    vector<string> _games;

    GameOptions _options;

    // Current game, its sgf record and game board state.
    int _curr_game;
    int _game_loaded;
    std::mt19937 _rng;

    bool next_move();
    const Sgf &pick_sgf();
    void reset(const Sgf &sgf);
    bool need_reload() const;
    void reload();

    std::string info() const {
        std::stringstream ss;
        Coord m = _sgf_iter.GetCoord();
        ss << _sgf_iter.GetCurrIdx() << "/" << _sgf_iter.GetSgf().NumMoves() << ": " << coord2str(m) << ", " << coord2str2(m) << " (" << m << ")" << std::endl;
        return ss.str();
    }

    void print_context() const {
        cout << "[curr_game=" << _curr_game << "][filename=" << _games[_curr_game] << "] " << info() << endl;
    }

    bool save_forward_moves(const BoardFeature &bf, vector<int64_t> *actions) const;

public:
    OfflineLoader(const GameOptions &options, int seed);
    static void InitSharedBuffer(const std::string &list_filename);

    bool Ready(const std::atomic_bool &done) override;
    void SaveTo(GameState &state) override;
    void Next(int64_t action) override;
};

class OnlinePlayer: public Loader {
public:
    OnlinePlayer() { }
    void SaveTo(GameState &state) override;
    void Next(int64_t action) override;
};
