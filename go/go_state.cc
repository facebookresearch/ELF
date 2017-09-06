#include <fstream>
#include <chrono>

#include "go_state.h"

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

void GoState::build_handicap_table() {
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

void GoState::Reset(const Sgf &sgf) {
    ClearBoard(&_board);
    _sgf_iter = sgf.begin();

    // Place handicap stones if there is any.
    int handi = sgf.GetHandicapStones();
    if (handi > 0) {
        auto it = _handicaps.find(handi);
        if (it != _handicaps.end()) {
            for (const auto& ha : it->second) {
                PlaceHandicap(&_board, X(ha), Y(ha), S_BLACK);
            }
        }
    }
}

bool GoState::NeedReload() const {
    return (_sgf_iter.done() || _sgf_iter.StepLeft() < NUM_FUTURE_ACTIONS);
}

bool GoState::NextMove() {
    GroupId4 ids;
    if (TryPlay2(&_board, _sgf_iter.GetCoord(), &ids)) {
      Play(&_board, &ids);
      ++ _sgf_iter;
      return true;
    } else {
      return false;
    }
}

bool GoState::GetForwardMoves(vector<SgfMove> *future_moves) const {
    assert(future_moves);
    *future_moves = _sgf_iter.GetForwardMoves(NUM_FUTURE_ACTIONS);
    return future_moves->size() >= NUM_FUTURE_ACTIONS;
}

static float *board_plane(vector<float> &features, int idx) {
    return &features[idx * BOARD_DIM * BOARD_DIM];
}

#define LAYER(idx) board_plane(state.features, idx)

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
#define OUR_CLOSEST_COLOR    14
#define OPPONENT_CLOSEST_COLOR   15

void GoState::SaveTo(GameState& state, const vector<SgfMove> &future_moves) const {
  Stone player = _board._next_player;

  state.features.resize(MAX_NUM_FEATURE * BOARD_DIM * BOARD_DIM);
  state.a.resize(NUM_FUTURE_ACTIONS);

  state.move_idx = _board._ply;
  Stone winner = _sgf_iter.GetSgf().GetWinner();
  state.winner = (winner == S_BLACK ? 1 : (winner == S_WHITE ? -1 : 0));

  // Save the current board state to game state.
  GetLibertyMap3binary(&_board, player, LAYER(OUR_LIB));
  GetLibertyMap3binary(&_board, OPPONENT(player), LAYER(OPPONENT_LIB));
  GetSimpleKo(&_board, player, LAYER(OUR_SIMPLE_KO));
  GetStones(&_board, player, LAYER(OUR_STONES));
  GetStones(&_board, OPPONENT(player), LAYER(OPPONENT_STONES));
  GetStones(&_board, S_EMPTY, LAYER(EMPTY_STONES));
  GetHistoryExp(&_board, player, LAYER(OUR_HISTORY));
  GetHistoryExp(&_board, OPPONENT(player), LAYER(OPPONENT_HISTORY));
  GetDistanceMap(&_board, player, LAYER(OUR_CLOSEST_COLOR));
  GetDistanceMap(&_board, OPPONENT(player), LAYER(OPPONENT_CLOSEST_COLOR));

  for (int i = 0; i < NUM_FUTURE_ACTIONS; ++i) {
      int action = future_moves[i].GetAction();
      if (action < 0 || action >= BOARD_DIM * BOARD_DIM) {
          Coord move = future_moves[i].move;
          Stone player = future_moves[i].player;
          // print_context();
          cout << "invalid action! action = " << action << " x = " << X(move) << " y = " << Y(move)
               << " player = " << player << " " << coord2str(move) << endl;
          action = 0;
      }
      state.a[i] = action;
  }
}

