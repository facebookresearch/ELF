/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "go_game_specific.h"
#include "go_state.h"
#include "elf/ai.h"

using AI = elf::AI_T<GoState, Coord>;
using AIWithComm = elf::AIWithCommT<GoState, Coord, AIComm>;
using AIHoldStateWithComm = elf::AIHoldStateWithCommT<GoState, Coord, AIComm>;
