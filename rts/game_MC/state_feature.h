#pragma once

#include "engine/game_state.h"

void MCExtract(const RTSState &s, PlayerId player_id, bool respect_fow, std::vector<float> *state);

