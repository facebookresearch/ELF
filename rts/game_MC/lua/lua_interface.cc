#include "lua_interface.h"

namespace detail {

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


}

void reg_lua_interfaces() {
    detail::_LuaCoord::Init();
    detail::_LuaRTSMap::Init();
    detail::_LuaMapSlot::Init();
}
