#pragma once

#include "elf/shared_replay_buffer.h"
#include "go_loader.h"
#include "elf/tar_loader.h"

using namespace std;

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
    OfflineLoader(const GameOptions &options, int seed, AIComm *ai_comm);
    static void InitSharedBuffer(const std::string &list_filename);

    bool Ready(const std::atomic_bool &done) override;
    void SaveTo(GameState &state) override;
    void Next(int64_t action) override;
};

