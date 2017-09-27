#pragma once

#include "go_game_specific.h"
#include "go_state.h"
#include "elf/ai.h"

using AI = elf::AI_T<GoState, Coord>;
using AIWithComm = elf::AIWithCommT<GoState, Coord, AIComm>;
