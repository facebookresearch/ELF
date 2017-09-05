#pragma once

#include "elf/pybind_helper.h"
#include "elf/comm_template.h"
#include "elf/ai_comm.h"
#include "elf/shared_replay_buffer.h"

#include "go_game_specific.h"
#include "board.h"
#include "sgf.h"
#include <random>
#include <map>
#include <sstream>

using Context = ContextT<GameOptions, HistT<GameState>>;
using Comm = typename Context::Comm;
using AIComm = AICommT<Comm>;

using RBuffer = SharedReplayBuffer<std::string, Sgf>;

// Game interface for Go.
class GoGame {
private:
    int _game_idx = -1;
    AIComm* _ai_comm = nullptr;
    GameOptions _options;

    std::mt19937 _rng;

    string _path;

    // Database
    vector<string> _games;

    // Current game, its sgf record and game board state.
    int _curr_game;
    Sgf::SgfIterator _sgf_iter;
    Board _board;

    // Shared Sgf buffer.
    RBuffer *_rbuffer = nullptr;

    // handicap table.
    map<int, vector<Coord> > _handicaps;

    void reload();
    void build_handicap_table();
    void print_context() const;

public:
    GoGame(int game_idx, const GameOptions& options);

    void Init(AIComm *ai_comm, RBuffer *rbuffer) {
        assert(ai_comm);
        assert(rbuffer);
        _ai_comm = ai_comm;
        _rbuffer = rbuffer;
    }

    void MainLoop(const std::atomic_bool& done) {
        // Main loop of the game.
        while (true) {
            Act(done);
            if (done.load()) break;
        }
    }

    void Act(const std::atomic_bool& done);
    void SaveTo(GameState &state, const vector<SgfMove> &future_moves) const;
};
