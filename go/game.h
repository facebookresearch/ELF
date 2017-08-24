#ifndef _GAME_H_
#define _GAME_H_

#include "elf/pybind_helper.h"
#include "elf/comm_template.h"
#include "elf/ai_comm.h"
#include "go_game_specific.h"
#include "board.h"
#include "sgf.h"
#include <random>
#include <map>
#include <sstream>

using Context = ContextT<GameOptions, HistT<GameState>>;
using Comm = typename Context::Comm;
using AIComm = AICommT<Comm>;

// Game interface for Go.
class GoGame {
private:
    int _game_idx = -1;
    AIComm* _ai_comm;
    GameOptions _options;

    std::mt19937 _rng;

    string _path;

    // Database
    vector<string> _games;

    // Current game, its sgf record and game board state.
    int _curr_game;
    Board _board;
    Sgf _sgf;

    // handicap table.
    map<int, vector<Coord> > _handicaps;

    void reload();
    void build_handicap_table();

public:
    GoGame(const GameOptions& options);
    void MainLoop(const std::atomic_bool& done) {
        // Main loop of the game.
        while (true) {
            Act(done);
            if (done.load()) break;
        }
    }

    void initialize_comm(int game_idx, AIComm* ai_comm) {
      assert(!_ai_comm);
      _ai_comm = ai_comm;
      _game_idx = game_idx;
    }

    void Act(const std::atomic_bool& done);
    void SaveTo(GameState &state, const vector<pair<Stone, Coord>> &future_moves) const;
};

#endif
