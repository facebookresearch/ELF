#pragma once

#include "elf/replay_loader.h"
#include "elf/tar_loader.h"
#include "ai.h"

using namespace std;

class OfflineLoader : public elf::ReplayLoaderT<std::string, Sgf>, public AIHoldStateWithComm {
public:
    using Data = typename AIHoldStateWithComm::Data;
    using ReplayLoader = elf::ReplayLoaderT<std::string, Sgf>; 

public:
    OfflineLoader(const GameOptions &options, int seed);
    static void InitSharedBuffer(const std::string &list_filename);

protected:
    // Database
    static std::unique_ptr<elf::TarLoader> _tar_loader;
    static vector<string> _games;
    static string _list_filename;
    static string _path;

    GameOptions _options;

    // Current game, its sgf record and game board state.
    int _curr_game;
    int _game_loaded;
    std::mt19937 _rng;

    // Virtual function for ReplayLoader:
    std::string get_key() override;
    bool after_reload(const std::string &full_name, Sgf::iterator &it) override;

    // Helper function.
    bool need_reload(const Sgf::iterator &it) const;
    void next();

    // Virtual function for AIHoldStateWithComm
    void before_act(const std::atomic_bool *) override { 
        if (s().JustStarted()) ai_comm()->Restart();
    }

    void extract(Data *data) override;
    bool handle_response(const Data &data, Coord *c) override;

    std::string info() const {
        std::stringstream ss;
        const auto &it = this->curr();
        Coord m = it.GetCoord();
        ss << it.GetCurrIdx() << "/" << it.GetSgf().NumMoves() << ": " << coord2str(m) << ", " << coord2str2(m) << " (" << m << ")" << std::endl;
        return ss.str();
    }

    void print_context() const {
        cout << "[curr_game=" << _curr_game << "][filename=" << _games[_curr_game] << "] " << info() << endl;
    }

    bool save_forward_moves(const BoardFeature &bf, vector<int64_t> *actions) const;
};

