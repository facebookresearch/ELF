#include "offpolicy_loader.h"

#include <fstream>

///////////// OfflineLoader ////////////////////
std::unique_ptr<TarLoader> OfflineLoader::_tar_loader;
std::unique_ptr<RBuffer> OfflineLoader::_rbuffer;

OfflineLoader::OfflineLoader(const GameOptions &options, int seed)
    : AIWithComm(&_state), _options(options), _game_loaded(0), _rng(seed) {
    if (_options.verbose) std::cout << "Loading list_file: " << _options.list_filename << std::endl;
    if (file_is_tar(_options.list_filename)) {
      // Get all .sgf from tar
      TarLoader tl = TarLoader(_options.list_filename.c_str());
      _games = tl.List();
    } else {
      // Get all .sgf file in the directory.
      ifstream iFile(_options.list_filename);
      _games.clear();
      for (string this_game; std::getline(iFile, this_game) ; ) {
        _games.push_back(this_game);
      }
      while (_games.back().empty()) _games.pop_back();

      // Get the path of the filename.
      _path = string(_options.list_filename);
      int i = _path.size() - 1;
      while (_path[i] != '/' && i >= 0) i --;

      if (i >= 0) _path = _path.substr(0, i + 1);
      else _path = "";
    }
    if (_options.verbose) std::cout << "Loaded: #Game: " << _games.size() << std::endl;
}

bool OfflineLoader::ready(const std::atomic_bool *done) {
  // Act on the current game.
  while ( need_reload() && (done == nullptr || !done->load()) ) {
      // std::cout << "Reloading games.." << std::endl;
      reload();
  }
  if (done->load()) return false;

  while (true) {
      if (_sgf_iter.StepLeft() >= _options.num_future_actions) break;
      print_context();
      cout << "future_moves.size() [" + std::to_string(_sgf_iter.StepLeft()) + "] < #FUTURE_MOVES [" + std::to_string(_options.num_future_actions) << endl;
      reload();
  }
  return true;
}

void OfflineLoader::InitSharedBuffer(const std::string &list_filename) {
    if (file_is_tar(list_filename)) {
        _tar_loader.reset(new TarLoader(list_filename));
    }
    TarLoader *tar_loader = _tar_loader.get();
    _rbuffer.reset(
            new RBuffer([tar_loader](const std::string &name) {
                std::unique_ptr<Sgf> sgf(new Sgf());
                if (tar_loader != nullptr) {
                    sgf->Load(name, *tar_loader);
                } else {
                    sgf->Load(name);
                }
                return sgf;
           }));
}

// Private functions.
void OfflineLoader::reset(const Sgf &sgf) {
    _state.Reset();
    _sgf_iter = sgf.begin();

    // Place handicap stones if there is any.
    int handi = sgf.GetHandicapStones();
    if (_options.verbose) std::cout << "#Handi = " << handi << std::endl;
    _state.ApplyHandicap(handi);

    _game_loaded ++;
}

const Sgf &OfflineLoader::pick_sgf() {
    while (true) {
        _curr_game = _rng() % _games.size();
        std::string full_name = file_is_tar(_list_filename) ? _games[_curr_game] : _path + _games[_curr_game];
        bool file_loaded = _rbuffer->HasKey(full_name);

        const auto &sgf = _rbuffer->Get(full_name);
        if (_options.verbose) {
            if (! file_loaded)
              std::cout << "Loaded file " << full_name << std::endl;
        }
        if (sgf.NumMoves() >= 10 && sgf.GetBoardSize() == BOARD_DIM) return sgf;
    }
}

void OfflineLoader::reload() {
    const Sgf &sgf = pick_sgf();
    reset(sgf);

    if (_options.verbose) print_context();

    // Then we need to randomly play the game.
    const float ratio_pre_moves = (_game_loaded == 1 ? _options.start_ratio_pre_moves : _options.ratio_pre_moves);

    int random_base = static_cast<int>(sgf.NumMoves() * ratio_pre_moves + 0.5);
    if (random_base == 0) random_base ++;
    int pre_moves = _rng() % random_base;

    for (int i = 0; i < pre_moves; ++i) next_move();

    if (_options.verbose) {
        std::cout << "PreMove: " << pre_moves << std::endl;
        print_context();
    }
}


bool OfflineLoader::need_reload() const {
    return (_sgf_iter.done() || _sgf_iter.StepLeft() < _options.num_future_actions 
            || (_options.move_cutoff >= 0 && _sgf_iter.GetCurrIdx() >= _options.move_cutoff));
}

bool OfflineLoader::next_move() {
    bool res = _state.forward(_sgf_iter.GetCoord());
    if (res) ++ _sgf_iter;
    return res;
}

void OfflineLoader::extract(Data *data) {
  auto& gs = data->newest();
  gs.game_record_idx = _curr_game;
  gs.move_idx = _state.GetPly();
  Stone winner = _sgf_iter.GetSgf().GetWinner();
  gs.winner = (winner == S_BLACK ? 1 : (winner == S_WHITE ? -1 : 0));

  int code = _options.data_aug;
  if (code  == -1 || code >= 8) code = _rng() % 8;
  gs.aug_code = code;

  auto rot = (BoardFeature::Rot)(code % 4);
  bool flip = (code >> 2) == 1;
  const BoardFeature &bf = _state.extractor(rot, flip);

  bf.Extract(&gs.s);
  save_forward_moves(bf, &gs.offline_a);
}

bool OfflineLoader::handle_response(const Data &data, Coord *c) {
    (void)data;
    *c = _sgf_iter.GetCoord();
    if (!next_move()) reload();
    return true;
}

bool OfflineLoader::save_forward_moves(const BoardFeature &bf, vector<int64_t> *actions) const {
    assert(actions);
    vector<SgfMove> future_moves = _sgf_iter.GetForwardMoves(_options.num_future_actions);
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

