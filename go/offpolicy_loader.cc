#include "offpolicy_loader.h"

#include <fstream>

///////////// OfflineLoader ////////////////////
std::unique_ptr<TarLoader> OfflineLoader::_tar_loader;
vector<string> OfflineLoader::_games;
string OfflineLoader::_list_filename;
string OfflineLoader::_path;

OfflineLoader::OfflineLoader(const GameOptions &options, int seed)
    : _options(options), _game_loaded(0), _rng(seed) {
}

void OfflineLoader::InitSharedBuffer(const std::string &list_filename) {
    if (file_is_tar(list_filename)) {
        _tar_loader.reset(new TarLoader(list_filename));
        _games = _tar_loader->List();
    } else {
        // Get all .sgf file in the directory.
        ifstream iFile(list_filename);
        _games.clear();
        for (string this_game; std::getline(iFile, this_game) ; ) {
            _games.push_back(this_game);
        }
        while (_games.back().empty()) _games.pop_back();

        // Get the path of the filename.
        _path = string(list_filename);
        int i = _path.size() - 1;
        while (_path[i] != '/' && i >= 0) i --;

        if (i >= 0) _path = _path.substr(0, i + 1);
        else _path = "";
    }
    _list_filename = list_filename;

    std::cout << "Loading list_file: " << list_filename << std::endl;
    std::cout << "Loaded: #Game: " << _games.size() << std::endl;

    TarLoader *tar_loader = _tar_loader.get();
    auto gen = [tar_loader](const std::string &name) {
                std::unique_ptr<Sgf> sgf(new Sgf());
                if (tar_loader != nullptr) {
                    sgf->Load(name, *tar_loader);
                } else {
                    sgf->Load(name);
                }
                return sgf;
           };
    ReplayLoader::Init(gen);
}

// Private functions.
bool OfflineLoader::after_reload(const std::string & /*full_name*/, Sgf::iterator &it) {
    const Sgf &sgf = it.GetSgf();
    /*
    if (_options.verbose) {
        std::cout << "Loaded file " << full_name << std::endl;
    }
    */
    // If the game record is too short or the boardsize is not what we want, return false.
    if (sgf.NumMoves() < 10 || sgf.GetBoardSize() != BOARD_DIM) return false;
    if (need_reload(it)) return false;

    // iterator valid, now we change the state accordingly.

    s().Reset();

    // Place handicap stones if there is any.
    int handi = sgf.GetHandicapStones();
    if (_options.verbose) std::cout << "#Handi = " << handi << std::endl;
    s().ApplyHandicap(handi);

    _game_loaded ++;

    if (_options.verbose) print_context();

    // Then we need to randomly play the game.
    const float ratio_pre_moves = (_game_loaded == 1 ? _options.start_ratio_pre_moves : _options.ratio_pre_moves);

    int random_base = static_cast<int>(sgf.NumMoves() * ratio_pre_moves + 0.5);
    if (random_base == 0) random_base ++;
    int pre_moves = _rng() % random_base;

    for (int i = 0; i < pre_moves; ++i) ReplayLoader::Next();

    if (_options.verbose) {
        std::cout << "PreMove: " << pre_moves << std::endl;
        print_context();
    }
    return true;
}

std::string OfflineLoader::get_key() {
    _curr_game = _rng() % _games.size();
    return file_is_tar(_list_filename) ? _games[_curr_game] : _path + _games[_curr_game];
}

bool OfflineLoader::need_reload(const Sgf::iterator &it) const {
   return (it.done() || it.StepLeft() < _options.num_future_actions 
            || (_options.move_cutoff >= 0 && it.GetCurrIdx() >= _options.move_cutoff));
}

bool OfflineLoader::before_next_action(const Sgf::iterator &it) {
    if (need_reload(it)) return false;

    bool res = s().forward(it.GetCoord());
    if (! res) return false;
    return true;
}

void OfflineLoader::extract(Data *data) {
    auto& gs = data->newest();
    gs.game_record_idx = _curr_game;
    gs.move_idx = s().GetPly();
    Stone winner = this->curr().GetSgf().GetWinner();
    gs.winner = (winner == S_BLACK ? 1 : (winner == S_WHITE ? -1 : 0));

    int code = _options.data_aug;
    if (code  == -1 || code >= 8) code = _rng() % 8;
    gs.aug_code = code;

    auto rot = (BoardFeature::Rot)(code % 4);
    bool flip = (code >> 2) == 1;
    const BoardFeature &bf = s().extractor(rot, flip);

    bf.Extract(&gs.s);
    save_forward_moves(bf, &gs.offline_a);
}

bool OfflineLoader::handle_response(const Data &data, Coord *c) {
    (void)data;
    *c = curr().GetCoord();
    ReplayLoader::Next();
    return true;
}

bool OfflineLoader::save_forward_moves(const BoardFeature &bf, vector<int64_t> *actions) const {
    assert(actions);
    vector<SgfMove> future_moves = curr().GetForwardMoves(_options.num_future_actions);
    if ((int)future_moves.size() < _options.num_future_actions) return false;

    actions->resize(_options.num_future_actions);
    for (int i = 0; i < _options.num_future_actions; ++i) {
        int action = bf.Coord2Action(future_moves[i].move);
        if (action < 0 || action >= BOARD_DIM * BOARD_DIM) {
            Coord move = future_moves[i].move;
            Stone player = future_moves[i].player;
            // print_context();
            cout << "invalid action! action = " << action << " x = " << X(move) << " y = " << Y(move)
                << " player = " << player << " " << coord2str(move) << endl;
            action = 0;
        }
        actions->at(i) =  action;
    }
    return true;
}

