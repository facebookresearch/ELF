#include <fstream>
#include <chrono>

#include "go_state.h"
#include "board_feature.h"

static std::vector<std::string> split(const std::string &s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (getline(ss, item, delim)) {
        elems.push_back(std::move(item));
    }
    return elems;
}

static Coord s2c(const string &s) {
    int row = s[0] - 'A';
    int col = stoi(s.substr(1)) - 1;
    return GetCoord(row, col);
}

HandicapTable::HandicapTable() {
    // darkforestGo/cnnPlayerV2/cnnPlayerV2Framework.lua
    // Handicap according to the number of stones.
    const map<int, string> handicap_table = {
        {2, "D4 Q16"},
        {3, "D4 Q16 Q4"},
        {4, "D4 Q16 D16 Q4"},
        {5, "*4 K10"},
        {6, "*4 D10 Q10"},
        {7, "*4 D10 Q10 K10"},
        {8, "*4 D10 Q10 K16 K4"},
        {9, "*8 K10"},
        // {13, "*9 G13 O13 G7 O7", "*9 C3 R3 C17 R17" },
    };
    for (const auto &pair : handicap_table) {
        _handicaps.insert(make_pair(pair.first, vector<Coord>()));
        for (const auto &s : split(pair.second, ' ')) {
            if (s[0] == '*') {
                const int prev_handi = stoi(s.substr(1));
                auto it = _handicaps.find(prev_handi);
                if (it != _handicaps.end()) {
                    _handicaps[pair.first] = it->second;
                }
            }
            _handicaps[pair.first].push_back(s2c(s));
        }
    }
}

void HandicapTable::Apply(int handi, Board *board) const {
    if (handi > 0) {
        auto it = _handicaps.find(handi);
        if (it != _handicaps.end()) {
            for (const auto& ha : it->second) {
                PlaceHandicap(board, X(ha), Y(ha), S_BLACK);
            }
        }
    }
}

///////////// GoState ////////////////////
bool GoState::ApplyMove(Coord c) {
    GroupId4 ids;
    if (TryPlay2(&_board, c, &ids)) {
      Play(&_board, &ids);
      return true;
    } else {
      return false;
    }
}

void GoState::Reset() {
    ClearBoard(&_board);
}

void GoState::ApplyHandicap(int handi) {
    _handi_table.Apply(handi, &_board);
}

///////////// OfflineLoader ////////////////////
std::unique_ptr<TarLoader> OfflineLoader::_tar_loader;
std::unique_ptr<RBuffer> OfflineLoader::_rbuffer;

OfflineLoader::OfflineLoader(const std::string &list_filename, int num_future_moves, bool verbose, int seed)
    : _num_future_moves(num_future_moves),  _verbose(verbose), _rng(seed) {
    if (verbose) std::cout << "Loading list_file: " << list_filename << std::endl;
    if (file_is_tar(list_filename)) {
      // Get all .sgf from tar
      TarLoader tl = TarLoader(list_filename.c_str());
      _games = tl.List();
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
    if (verbose) std::cout << "Loaded: #Game: " << _games.size() << std::endl;
}

bool OfflineLoader::Ready(const std::atomic_bool &done) {
  // Act on the current game.
  while ( need_reload() && !done.load() ) {
      // std::cout << "Reloading games.." << std::endl;
      reload();
  }
  if (done.load()) return false;

  while (true) {
      if (_sgf_iter.StepLeft() >= _num_future_moves) break;
      print_context();
      cout << "future_moves.size() [" + std::to_string(_sgf_iter.StepLeft()) + "] < #FUTURE_MOVES [" + std::to_string(_num_future_moves) << endl;
      reload();
  }
  return true;
}

void OfflineLoader::Next(int64_t action) {
    (void)action;
    if (!next_move()) reload();
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
    _state.ApplyHandicap(sgf.GetHandicapStones());
}

const Sgf &OfflineLoader::pick_sgf() {
    while (true) {
        _curr_game = _rng() % _games.size();
        // std::cout << "_game_idx = " << _curr_game << std::endl;

        std::string full_name = file_is_tar(_list_filename) ? _games[_curr_game] : _path + _games[_curr_game];
        // std::cout << "full_name = " << full_name << std::endl;
        //
        bool file_loaded = _rbuffer->HasKey(full_name);
        // std::cout << "Has key: " << (file_loaded ? "True" : "False") << std::endl;

        const auto &sgf = _rbuffer->Get(full_name);
        if (_verbose) {
            if (! file_loaded)
              std::cout << "Loaded file " << full_name << std::endl;
        }
        if (sgf.NumMoves() >= 10 && sgf.GetBoardSize() == BOARD_DIM) return sgf;
    }
}

void OfflineLoader::reload() {
    const Sgf &sgf = pick_sgf();
    reset(sgf);

    if (_verbose) print_context();

    // Then we need to randomly play the game.
    int pre_moves = _rng() % (sgf.NumMoves() / 2);
    for (int i = 0; i < pre_moves; ++i) next_move();
}


bool OfflineLoader::need_reload() const {
    return (_sgf_iter.done() || _sgf_iter.StepLeft() < _num_future_moves);
}

bool OfflineLoader::next_move() {
    bool res = _state.ApplyMove(_sgf_iter.GetCoord());
    if (res) ++ _sgf_iter;
    return res;
}

void OfflineLoader::SaveTo(GameState& gs) {
  gs.move_idx = _state.GetPly();
  Stone winner = _sgf_iter.GetSgf().GetWinner();
  gs.winner = (winner == S_BLACK ? 1 : (winner == S_WHITE ? -1 : 0));

  auto rot = (BoardFeature::Rot)(_rng() % 4);
  bool flip = _rng() % 2 == 1;

  const auto &bf = _state.extractor(rot, flip);
  bf.Extract(&gs.s);
  save_forward_moves(bf, &gs.offline_a);
}

bool OfflineLoader::save_forward_moves(const BoardFeature &bf, vector<int64_t> *actions) const {
    assert(actions);
    vector<SgfMove> future_moves = _sgf_iter.GetForwardMoves(_num_future_moves);
    if ((int)future_moves.size() < _num_future_moves) return false;

    actions->resize(_num_future_moves);
    for (int i = 0; i < _num_future_moves; ++i) {
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

void OnlinePlayer::SaveTo(GameState &gs) {
    gs.move_idx = _state.GetPly();
    gs.winner = 0;
    const auto &bf = _state.extractor();
    bf.Extract(&gs.s);
}

void OnlinePlayer::Next(int64_t action) {
    // From action to coord.
    Coord m = _state.last_extractor().Action2Coord(action);
    // Play it.
    if (! _state.ApplyMove(m)) {
        _state.Reset();
    }
}
