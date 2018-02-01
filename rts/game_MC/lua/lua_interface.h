#pragma once

#include "elf/lua/interface.h"

#include "engine/common.h"
#include "engine/map.h"


namespace detail {

struct _LuaCoord : public LuaInterface<Coord> {
    static void Init();
};

struct _LuaRTSMap : public LuaInterface<RTSMap> {
    static void Init();
};

struct _LuaMapSlot : public LuaInterface<MapSlot> {
    static void Init();
};


}

void reg_lua_interfaces();
