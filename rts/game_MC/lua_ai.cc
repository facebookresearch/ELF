#include "lua_ai.h"

bool LuaAI::on_act(Tick, RTSMCAction *a, const std::atomic_bool *) {
    a->Init(id(), name());
    a->SetLuaAI();
    return true;
}
