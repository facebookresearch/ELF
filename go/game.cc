#include "game.h"
#include "go_game_specific.h"

#include <fstream>
#include <chrono>

GoGame::GoGame(int game_idx, const GameOptions& options) : _options(options) {
    _game_idx = game_idx;
    uint64_t seed = 0;
    if (options.seed == 0) {
        auto now = chrono::system_clock::now();
        auto now_ms = chrono::time_point_cast<chrono::milliseconds>(now);
        auto value = now_ms.time_since_epoch();
        long duration = value.count();
        seed = (time(NULL) * 1000 + duration + _game_idx * 2341479) % 100000000;
        if (_options.verbose) std::cout << "[" << _game_idx << "] Seed:" << seed << std::endl;
    } else {
        seed = options.seed;
    }
    _rng.seed(seed);

    build_handicap_table();

    // Get all .sgf file in the directory.
    if (_options.verbose) std::cout << "[" << _game_idx << "] Loading list_file: " << options.list_filename << std::endl;
    ifstream iFile(options.list_filename);
    _games.clear();
    for (string this_game; std::getline(iFile, this_game) ; ) {
      _games.push_back(this_game);
    }
    while (_games.back().empty()) _games.pop_back();
    if (_options.verbose) std::cout << "[" << _game_idx << "] Loaded: #Game: " << _games.size() << std::endl;

    // Get the path of the filename.
    _path = string(options.list_filename);
    int i = _path.size() - 1;
    while (_path[i] != '/' && i >= 0) i --;

    if (i >= 0) _path = _path.substr(0, i + 1);
    else _path = "";

    if (_options.verbose) std::cout << "[" << _game_idx << "] Done with initialization" << std::endl;
}

void GoGame::print_context() const {
    cout << "[id=" << _game_idx << "][curr_game=" << _curr_game << "][filename="
         << _games[_curr_game] << " " << _sgf_iter.GetCurrIdx() << "/" << _sgf_iter.GetSgf().NumMoves() << endl;
}

void GoGame::Act(const std::atomic_bool& done) {
  // Act on the current game.
  while ( (_sgf_iter.done() || _sgf_iter.StepLeft() < NUM_FUTURE_ACTIONS) && !done.load() ) {
      // std::cout << "Reloading games.." << std::endl;
      reload();
  }
  if (done.load()) return;

  vector<SgfMove> future_moves;
  while (true) {
      future_moves = _sgf_iter.GetForwardMoves(NUM_FUTURE_ACTIONS);
      if (future_moves.size() >= NUM_FUTURE_ACTIONS) break;
      print_context();
      cout << "future_moves.size() [" +
          std::to_string(future_moves.size()) + "] < #FUTURE_ACTIONS [" + std::to_string(NUM_FUTURE_ACTIONS) << endl;
      reload();
  }
  //bool terminal = (_sgf.StepLeft() == NUM_FUTURE_ACTIONS);

  // Send the current board situation.
  auto& gs = _ai_comm->Prepare();
  //meta->tick = _board._ply;
  //meta->terminated = terminal;
  //if (terminal) meta->winner = _sgf.GetWinner();
  SaveTo(gs, future_moves);

  // There is always only 1 player.
  _ai_comm->SendDataWaitReply();

  GroupId4 ids;
  if (TryPlay2(&_board, future_moves[0].move, &ids)) {
      Play(&_board, &ids);
      ++ _sgf_iter;
  } else {
      reload();
  }
}

/* darkforestGo/utils/goutils.lua
extended = {
    "our liberties", "opponent liberties", "our simpleko", "our stones", "opponent stones", "empty stones", "our history", "opponent history",
    "border", 'position_mask', 'closest_color'
},
*/

#define OUR_LIB          0
#define OPPONENT_LIB     3
#define OUR_SIMPLE_KO    6
#define OUR_STONES       7
#define OPPONENT_STONES  8
#define EMPTY_STONES     9

// [TODO]: Other todo features.
#define OUR_HISTORY      10
#define OPPONENT_HISTORY 11
#define BORDER           12
#define POSITION_MARK    13
#define CLOSEST_COLOR    14

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

void GoGame::build_handicap_table() {
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
    _handicaps.clear();
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

void GoGame::reload() {
    while (true) {
        _curr_game = _rng() % _games.size();
        // std::cout << "_game_idx = " << _curr_game << std::endl;

        std::string full_name = _path + _games[_curr_game];
        // std::cout << "full_name = " << full_name << std::endl;

        bool file_loaded = _rbuffer->HasKey(full_name);
        // std::cout << "Has key: " << (file_loaded ? "True" : "False") << std::endl;

        const auto &sgf = _rbuffer->Get(full_name);
        if (_options.verbose) {
            if (! file_loaded)
              std::cout << "Loaded file " << full_name << std::endl;
        }
        if (sgf.NumMoves() >= 10 && sgf.GetBoardSize() == BOARD_DIM) {
            _sgf_iter = sgf.begin();
            break;
        }
    }

    if (_options.verbose) print_context();

    // Clear the board.
    ClearBoard(&_board);
    _ai_comm->Restart();

    // Place handicap stones if there is any.
    const Sgf &sgf = _sgf_iter.GetSgf();
    int handi = sgf.GetHandicapStones();
    if (handi > 0) {
        auto it = _handicaps.find(handi);
        if (it != _handicaps.end()) {
            for (const auto& ha : it->second) {
                PlaceHandicap(&_board, X(ha), Y(ha), S_BLACK);
            }
        }
    }
    // Then we need to randomly play the game.
    int pre_moves = _rng() % (sgf.NumMoves() / 2);
    for (int i = 0; i < pre_moves; ++i) ++ _sgf_iter;
}

static float *board_plane(vector<float> &features, int idx) {
    return &features[idx * BOARD_DIM * BOARD_DIM];
}

#define LAYER(idx) board_plane(state.features, idx)

void GoGame::SaveTo(GameState& state, const vector<SgfMove> &future_moves) const {
  Stone player = _board._next_player;

  state.features.resize(MAX_NUM_FEATURE * BOARD_DIM * BOARD_DIM);
  state.a.resize(NUM_FUTURE_ACTIONS);

  // Save the current board state to game state.
  GetLibertyMap3(&_board, player, LAYER(OUR_LIB));
  GetLibertyMap3(&_board, OPPONENT(player), LAYER(OPPONENT_LIB));
  GetSimpleKo(&_board, player, LAYER(OUR_SIMPLE_KO));
  GetStones(&_board, player, LAYER(OUR_STONES));
  GetStones(&_board, OPPONENT(player), LAYER(OPPONENT_STONES));
  GetStones(&_board, S_EMPTY, LAYER(EMPTY_STONES));

  for (int i = 0; i < NUM_FUTURE_ACTIONS; ++i) {
      int action = future_moves[i].GetAction();
      if (action < 0 || action >= BOARD_DIM * BOARD_DIM) {
          Coord move = future_moves[i].move;
          Stone player = future_moves[i].player;
          print_context();
          cout << "invalid action! action = " << action << " x = " << X(move) << " y = " << Y(move)
               << " player = " << player << " " << coord2str(move) << endl;
          action = 0;
      }
      state.a[i] = action;
  }
}
