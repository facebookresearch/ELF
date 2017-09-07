#include "board_feature.h"
#include <utility>
#include <cmath>
using namespace std;

#define S_ISA(c1, c2) ( (c2 == S_EMPTY) || (c1 == c2) )
// For feature extraction.
// Distance transform
static void DistanceTransform(float* arr) {
#define IND(i, j) ((i) * BOARD_SIZE + (j))
  // First dimension.
  for (int j = 0; j < BOARD_SIZE; j++) {
    for (int i = 1; i < BOARD_SIZE; i++) {
      arr[IND(i, j)] = min(arr[IND(i, j)], arr[IND(i - 1, j)] + 1);
    }
    for (int i = BOARD_SIZE - 2; i >= 0; i--) {
      arr[IND(i, j)] = min(arr[IND(i, j)], arr[IND(i + 1, j)] + 1);
    }
  }
  // Second dimension
  for (int i = 0; i < BOARD_SIZE; i++) {
    for (int j = 1; j < BOARD_SIZE; j++) {
      arr[IND(i, j)] = min(arr[IND(i, j)], arr[IND(i, j - 1)] + 1);
    }
    for (int j = BOARD_SIZE - 2; j >= 0; j--) {
      arr[IND(i, j)] = min(arr[IND(i, j)], arr[IND(i, j + 1)] + 1);
    }
  }
#undef IDX
}

// If we set player = 0 (S_EMPTY), then the liberties of both side will be returned.
bool BoardFeature::GetLibertyMap(Stone player, float* data) const {
  // We assume the output liberties is a 19x19 tensor.
  /*
  if (THTensor_nDimension(liberties) != 2) return false;
  if (THTensor_size(liberties, 1) != BOARD_SIZE) return false;
  if (THTensor_size(liberties, 2) != BOARD_SIZE) return false;
  float *data = THTensor_data(liberties);

  int stride = THTensor_stride(liberties, 1);
  */
  memset(data, 0, BOARD_SIZE * BOARD_SIZE * sizeof(float));
  for (int i = 1; i < _board->_num_groups; ++i) {
    if (S_ISA(_board->_groups[i].color, player)) {
      int liberty = _board->_groups[i].liberties;
      TRAVERSE(_board, i, c) {
        data[transform(c)] = liberty;
      } ENDTRAVERSE
    }
  }

  return true;
}

bool BoardFeature::GetLibertyMap3(Stone player, float* data) const {
  // We assume the output liberties is a 3x19x19 tensor.
  // == 1, == 2, >= 3
  memset(data, 0, 3 * BOARD_SIZE * BOARD_SIZE * sizeof(float));
  for (int i = 1; i < _board->_num_groups; ++i) {
    if (S_ISA(_board->_groups[i].color, player)) {
      int liberty = _board->_groups[i].liberties;
      TRAVERSE(_board, i, c) {
        if (liberty == 1) data[transform(c, 0)] = liberty;
        else if (liberty == 2) data[transform(c, 1)] = liberty;
        else data[transform(c, 2)] = liberty;
      } ENDTRAVERSE
    }
  }

  return true;
}


bool BoardFeature::GetLibertyMap3binary(Stone player, float* data) const {
  // We assume the output liberties is a 3x19x19 tensor.
  // == 1, == 2, >= 3
  memset(data, 0, 3 * BOARD_SIZE * BOARD_SIZE * sizeof(float));
  for (int i = 1; i < _board->_num_groups; ++i) {
    if (S_ISA(_board->_groups[i].color, player)) {
      int liberty = _board->_groups[i].liberties;
      TRAVERSE(_board, i, c) {
        if (liberty == 1) data[transform(c, 0)] = 1.0;
        else if (liberty == 2) data[transform(c, 1)] = 1.0;
        else data[transform(c, 2)] = 1.0;
      } ENDTRAVERSE
    }
  }

  return true;
}


bool BoardFeature::GetStones(Stone player, float *data) const {
  memset(data, 0, BOARD_SIZE * BOARD_SIZE * sizeof(float));
  for (int i = 0; i < BOARD_SIZE; ++i) {
    for (int j = 0; j < BOARD_SIZE; ++j) {
      Coord c = OFFSETXY(i, j);
      if (_board->_infos[c].color == player) data[transform(i, j)] = 1;
    }
  }
  return true;
}

bool BoardFeature::GetSimpleKo(Stone /*player*/, float *data) const {
  memset(data, 0, BOARD_SIZE * BOARD_SIZE * sizeof(float));
  Coord m = GetSimpleKoLocation(_board, NULL);
  if (m != M_PASS) {
    data[transform(m)] = 1;
    return true;
  }
  return false;
}

// If player == S_EMPTY, get history of both sides.
bool BoardFeature::GetHistory(Stone player, float *data) const {
  memset(data, 0, BOARD_SIZE * BOARD_SIZE * sizeof(float));
  for (int i = 0; i < BOARD_SIZE; ++i) {
    for (int j = 0; j < BOARD_SIZE; ++j) {
      Coord c = OFFSETXY(i, j);
      if (S_ISA(_board->_infos[c].color, player)) data[transform(i, j)] = _board->_infos[c].last_placed;
    }
  }
  return true;
}

bool BoardFeature::GetHistoryExp(Stone player, float *data) const {
  memset(data, 0, BOARD_SIZE * BOARD_SIZE * sizeof(float));
  for (int i = 0; i < BOARD_SIZE; ++i) {
    for (int j = 0; j < BOARD_SIZE; ++j) {
      Coord c = OFFSETXY(i, j);
      if (S_ISA(_board->_infos[c].color, player)) {
          data[transform(i, j)] = exp( (_board->_infos[c].last_placed - _board->_ply) / 10.0 );
      }
    }
  }
  return true;
}

bool BoardFeature::GetDistanceMap(Stone player, float *data) const {
  for (int i = 0; i < BOARD_SIZE; ++i) {
    for (int j = 0; j < BOARD_SIZE; ++j) {
      Coord c = OFFSETXY(i, j);
      if (_board->_infos[c].color == player)
        data[transform(i, j)] = 0;
      else
        data[transform(i, j)] = 10000;
    }
  }
  DistanceTransform(data);
  return true;
}

static float *board_plane(vector<float> *features, int idx) {
    return &(*features)[idx * BOARD_SIZE * BOARD_SIZE];
}

#define LAYER(idx) board_plane(features, idx)

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

void BoardFeature::Extract(std::vector<float> *features) const {
  Stone player = _board->_next_player;

  features->resize(MAX_NUM_FEATURE * BOARD_SIZE * BOARD_SIZE);
  std::fill(features->begin(), features->end(), 0.0);

  // Save the current board state to game state.
  GetLibertyMap3binary(player, LAYER(OUR_LIB));
  GetLibertyMap3binary(OPPONENT(player), LAYER(OPPONENT_LIB));
  GetSimpleKo(player, LAYER(OUR_SIMPLE_KO));
  GetStones(player, LAYER(OUR_STONES));
  GetStones(OPPONENT(player), LAYER(OPPONENT_STONES));
  GetStones(S_EMPTY, LAYER(EMPTY_STONES));
  GetHistoryExp(player, LAYER(OUR_HISTORY));
  GetHistoryExp(OPPONENT(player), LAYER(OPPONENT_HISTORY));
  GetDistanceMap(player, LAYER(OUR_CLOSEST_COLOR));
  GetDistanceMap(OPPONENT(player), LAYER(OPPONENT_CLOSEST_COLOR));
}

