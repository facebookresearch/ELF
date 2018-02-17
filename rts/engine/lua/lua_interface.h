#pragma once

#include "elf/lua/interface.h"

#include "utils.h"

#include "engine/common.h"
#include "engine/map.h"
#include "engine/gamedef.h"
#include "engine/unit.h"
#include "engine/cmd_specific.gen.h"
#include "engine/cmd_receiver.h"



namespace detail {

struct _LuaStateProxy : public LuaClassInterface<StateProxy> {
    static void Init();
};

struct _LuaCoord : public LuaClassInterface<Coord> {
    static void Init();
};

struct _LuaRTSMap : public LuaClassInterface<RTSMap> {
    static void Init();
};

struct _LuaMapSlot : public LuaClassInterface<MapSlot> {
    static void Init();
};


struct _LuaTerrain : public LuaEnumInterface<Terrain> {
    static void Init();
};

struct _LuaUnitType : public LuaEnumInterface<UnitType> {
    static void Init();
};

struct _LuaUnitAttr : public LuaEnumInterface<UnitAttr> {
    static void Init();
};

struct _LuaCDType : public LuaEnumInterface<CDType> {
    static void Init();
};

struct _LuaCmdType : public LuaEnumInterface<CmdType> {
    static void Init();
};

struct _LuaPointF : public LuaClassInterface<PointF> {
    static void Init();
};

struct _LuaUnit : public LuaClassInterface<Unit> {
    static void Init();
};

struct _LuaUnitProperty : public LuaClassInterface<UnitProperty> {
    static void Init();
};

struct _LuaUnitTemplate : public LuaClassInterface<UnitTemplate> {
    static void Init();
};

struct _LuaCooldown : public LuaClassInterface<Cooldown> {
    static void Init();
};

struct _LuaBuildSkill : public LuaClassInterface<BuildSkill> {
    static void Init();
};

}

void reg_engine_lua_interfaces();
