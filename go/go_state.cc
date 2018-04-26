/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

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
    if (row >= 9) row --;
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
bool GoState::forward(const Coord &c) {
    GroupId4 ids;
    if (! TryPlay2(&_board, c, &ids)) return false;
    Play(&_board, &ids);
    return true;
}

bool GoState::CheckMove(const Coord &c) const {
    GroupId4 ids;
    return TryPlay2(&_board, c, &ids);
}

void GoState::ApplyHandicap(int handi) {
    _handi_table.Apply(handi, &_board);
}

void GoState::Reset() {
    ClearBoard(&_board);
}

HandicapTable GoState::_handi_table;
