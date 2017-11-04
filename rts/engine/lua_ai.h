#pragma oncef
#include "ai.h"

// Lua AI, rule-based AI for Mini-RTS
class LuaAI : public AI {
public:
    LuaAI(const AIOptions &opt) : AI(opt.name, opt.fs)  { }

    // SERIALIZER_DERIVED(LuaAI, AIBase, _state);

private:
    bool on_act(Tick, RTSMCAction *action, const atomic_bool *) override;
};

