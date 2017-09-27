#pragma once

#include "elf/shared_replay_buffer.h"
#include "elf/tar_loader.h"
#include "ai.h"

using namespace std;

using RBuffer = SharedReplayBuffer<std::string, Sgf>;

class OfflineLoader : public AIWithComm {
public:
    using Data = AIWithComm::Data;

public:
    OfflineLoader(const GameOptions &options, int seed);
    static void InitSharedBuffer(const std::string &list_filename);

protected:
    // Shared buffer for OfflineLoader.
    static std::unique_ptr<RBuffer> _rbuffer;
    static std::unique_ptr<TarLoader> _tar_loader;

    GoState _state;

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

    void before_act(elf::Tick, const std::atomic_bool *done) override { 
        if (! ready(done)) return;
        if (s().JustStarted()) ai_comm()->Restart();
    }

    void extract(Data *data) override;
    bool handle_response(const Data &data, Coord *c) override;
    void on_set_state() override {
        cout << "OfflineLoader does not have SetState!" << endl;
        throw std::range_error("OfflineLoader has no SetState()");
    }

    bool ready(const std::atomic_bool *done);

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
};

