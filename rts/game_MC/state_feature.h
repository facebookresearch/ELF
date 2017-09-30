#pragma once

#include "engine/game_state.h"

#define _OFFSET(_c, _x, _y, _m) (((_c) * _m.GetYSize() + (_y)) * _m.GetXSize() + (_x))
#define _XY(loc, m) ((loc) % m.GetXSize()), ((loc) / m.GetXSize())

#define NUM_RES_SLOT 5

void MCExtract(const RTSState &s, PlayerId player_id, bool respect_fow, std::vector<float> *state);

