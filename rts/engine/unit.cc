/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "unit.h"
#include <sstream>

// -----------------------  Unit definition ----------------------
PlayerId Unit::GetPlayerId() const {
    return Player::ExtractPlayerId(_id);
}

string Unit::PrintInfo(const RTSMap&) const {
    // Print the information for the unit.
    stringstream ss;
    ss << "U[" << Player::ExtractPlayerId(_id) << ":" << _id << "], @(" << _p.x << ", " << _p.y << "), ";
    ss << _type << ", H " << _property._hp << "/" << _property._max_hp << ", B " << _built_since << " A" << _property._att << " D" << _property._def << " | ";
    for (int j = 0; j < NUM_COOLDOWN; j++) {
        const auto &cd = _property.CD((CDType)j);
        ss << (CDType)j << ": " << cd._cd << "/" << cd._last << "  ";
    }
    return ss.str();
}

string Unit::Draw(Tick tick) const {
    // Draw the unit.
    return make_string("c", Player::ExtractPlayerId(_id), _last_p, _p, _type) + " " + _property.Draw(tick);
}
