#include "lua_interface.h"

namespace detail {

void _LuaStateProxy::Init() {
    Register("StateProxy",
        "send_cmd_create", &StateProxy::SendCmdCreate,
        "send_cmd_change_player_resource", &StateProxy::SendCmdChangePlayerResource,
        "get_x_size", &StateProxy::GetXSize,
        "get_y_size", &StateProxy::GetYSize,
        "get_slot_type", &StateProxy::GetSlotType
    );
}

void _LuaCoord::Init() {
    Register("Coord",
        "x", &Coord::x,
        "y", &Coord::y,
        "z", &Coord::z
    );
}

void _LuaRTSMap::Init() {
    Register("RTSMap",
        "get_x_size", &RTSMap::GetXSize,
        "get_y_size", &RTSMap::GetYSize,
        //"level", &RTSMap::_level,
        "get_loc", &RTSMap::GetLoc,
        "get_coord", &RTSMap::GetCoord,
        "init_map", &RTSMap::InitMap,
        "set_slot_type", &RTSMap::SetSlotType,
        "get_slot_type", &RTSMap::GetSlotType,
        "reset_intermediates", &RTSMap::ResetIntermediates,
        "clear_map", &RTSMap::ClearMap,
        "add_player", &RTSMap::AddPlayer
    );
}

void _LuaMapSlot::Init() {
    Register("MapSlot",
        "height", &MapSlot::height
    );
}

void _LuaTerrain::Init() {
    Register("Terrain", _Terrain2string, WATER, INVALID_TERRAIN);
}

void _LuaUnitType::Init() {
    Register("UnitType", _UnitType2string, TOWN_HALL, INVALID_UNITTYPE);
}

void _LuaUnitAttr::Init() {
    Register("UnitAttr", _UnitAttr2string, ATTR_INVULNERABLE, INVALID_UNITATTR);
}

void _LuaCDType::Init() {
    Register("CDType", _CDType2string, NUM_COOLDOWN, INVALID_CDTYPE);
}

void _LuaCmdType::Init() {
    Register("CmdType", "MOVE", MOVE);
    Register("CmdType", "ATTACK", ATTACK);
    Register("CmdType", "BUILD", BUILD);
    Register("CmdType", "GATHER", GATHER);
}

void _LuaPointF::Init() {
    Register("PointF",
        "isvalid", &PointF::IsValid,
        "info", &PointF::info,
        "set_int_xy", &PointF::SetIntXY
    );
}

void _LuaUnit::Init() {
    Register("Unit",
        //"get_point", &Unit::GetPointF,
        //"get_property", &Unit::GetProperty,
        "get_id", &Unit::GetId,
        "get_player_id", &Unit::GetPlayerId,
        "get_unit_type", &Unit::GetUnitType
    );
}

void _LuaUnitProperty::Init() {
    Register("UnitProperty",
        "is_dead", &UnitProperty::IsDead,
        "hp", &UnitProperty::_hp,
        "max_hp", &UnitProperty::_max_hp,
        "att", &UnitProperty::_att,
        "def", &UnitProperty::_def,
        "att_r", &UnitProperty::_att_r,
        "vis_r", &UnitProperty::_vis_r,
        "set_speed", &UnitProperty::SetSpeed,
        "set_attr", &UnitProperty::SetAttr,
        "set_cooldown", &UnitProperty::SetCooldown
    );
}

void _LuaUnitTemplate::Init() {
    Register("UnitTemplate",
        "set_property", &UnitTemplate::SetProperty,
        "build_cost", &UnitTemplate::_build_cost,
        "add_allowed_cmd", &UnitTemplate::AddAllowedCmd,
        "set_attack_multiplier", &UnitTemplate::SetAttackMultiplier,
        "add_cant_move_over", &UnitTemplate::AddCantMoveOver,
        "set_build_from", &UnitTemplate::SetBuildFrom,
        "add_build_skill", &UnitTemplate::AddBuildSkill
    );
}

void _LuaCooldown::Init() {
    Register("Cooldown",
        "last", &Cooldown::_last,
        "cd", &Cooldown::_cd,
        "set", &Cooldown::Set,
        "passed", &Cooldown::Passed,
        "start", &Cooldown::Start
    );
}

void _LuaBuildSkill::Init() {
    Register("BuildSkill",
        "unit_type", &BuildSkill::_unit_type,
        "hotkey", &BuildSkill::_hotkey
    );
}

}

void reg_engine_lua_interfaces() {
    detail::_LuaStateProxy::Init();
    detail::_LuaCoord::Init();
    detail::_LuaRTSMap::Init();
    detail::_LuaMapSlot::Init();
    detail::_LuaPointF::Init();
    detail::_LuaUnit::Init();
    detail::_LuaUnitProperty::Init();
    detail::_LuaUnitTemplate::Init();
    detail::_LuaCooldown::Init();
    detail::_LuaBuildSkill::Init();

    detail::_LuaTerrain::Init();
    detail::_LuaUnitType::Init();
    detail::_LuaUnitAttr::Init();
    detail::_LuaCDType::Init();
    detail::_LuaCmdType::Init();
}
