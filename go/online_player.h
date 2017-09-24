#pragma once

#include "go_game_specific.h"
#include "go_loader.h"

class OnlinePlayer: public Loader {
public:
    OnlinePlayer(AIComm *ai_comm) : Loader(ai_comm) { }
    void SaveTo(GameState &state) override;
    void Next(int64_t action) override;
};
