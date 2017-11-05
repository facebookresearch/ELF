#include "lua_env.h"

LuaEnv::LuaEnv(const GameEnv &env) {
    _s.Load("test2.lua");
    _s["Env"].SetClass<LuaEnv, Unit>("units", &LuaEnv::GetUnit);
    _s["Unit"].SetClass<Unit, 
}

