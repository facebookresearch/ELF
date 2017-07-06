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
    ss << _type << " " << _p << " HP: " << _property.PrintInfo();
    return ss.str();
}

string Unit::Draw(Tick tick) const {
    // Draw the unit.
    return make_string("c", Player::ExtractPlayerId(_id), _last_p, _p, _type) + " " + _property.Draw(tick);
}
