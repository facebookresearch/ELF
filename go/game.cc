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
        // std::cout << "Seed:" << seed << std::endl;
    } else {
        seed = options.seed;
    }
    _rng.seed(seed);

    build_handicap_table();

    // Get all .sgf file in the directory.
    ifstream iFile(options.list_filename);
    _games.clear();
    while (! iFile.eof()) {
      string this_game;
      iFile >> this_game;
      _games.push_back(this_game);
    }
    _games.pop_back();
    // Get the path of the filename.
    _path = string(options.list_filename);
    int i = _path.size() - 1;
    while (_path[i] != '/') i --;

    // Include "/"
    _path = _path.substr(0, i + 1);
}

void GoGame::Act(const std::atomic_bool& done) {
  // Act on the current game.
  while ((_sgf.StepLeft() < NUM_FUTURE_ACTIONS) && !done.load() )
      reload();
  if (done.load()) {
      return;
  }
  vector<SgfMove> future_moves = _sgf.GetForwardMoves(NUM_FUTURE_ACTIONS);
  if (future_moves.size() < NUM_FUTURE_ACTIONS) {
      throw std::range_error("future_moves.size() [" +
          std::to_string(future_moves.size()) + "] < #FUTURE_ACTIONS [" + std::to_string(NUM_FUTURE_ACTIONS));
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
      _sgf.Next();
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
    do {
      _curr_game = _rng() % _games.size();
      // Load sgf file.
    } while (! _sgf.Load(_path + _games[_curr_game]) || _sgf.NumMoves() < 10);

    if (_options.verbose) {
        cout << "[" << _game_idx << "] New game: [" << _curr_game << "] "
             << _games[_curr_game] << " #Moves: " << _sgf.NumMoves() << endl;
    }

    // Clear the board.
    ClearBoard(&_board);
    _ai_comm->Restart();

    // Place handicap stones if there is any.
    int handi = _sgf.GetHandicapStones();
    if (handi > 0) {
        auto it = _handicaps.find(handi);
        if (it != _handicaps.end()) {
            for (const auto& ha : it->second) {
                PlaceHandicap(&_board, X(ha), Y(ha), S_BLACK);
            }
        }
    }
    // Then we need to randomly play the game.
    int pre_moves = _rng() % (_sgf.NumMoves() / 2);
    for (int i = 0; i < pre_moves; ++i) _sgf.Next();
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
      state.a[i] = future_moves[i].GetAction();
  }
}
