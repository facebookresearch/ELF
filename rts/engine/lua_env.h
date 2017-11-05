#pragma once
#include "game_env.h"
#include "cmd_receiver.h"
#include "lua.hpp"
#include "selene.h"

class LuaEnv {
public:
    LuaEnv();
private:
    sel::State _s;
};
