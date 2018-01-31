#include "map.h"


sel::State LuaContext::state_{true};

std::mutex LuaContext::mutex_;

void LuaContext::RegisterLuaFunc(const std::string& file_name) {
  state_.Load(file_name);
}

void reg_lua_context() {
  // Register Types
  LuaContext::RegisterCppType<MapSlot>("MapSlot");
  LuaContext::RegisterCppType<RTSMap>("RTSMap");
  LuaContext::RegisterCppType<RTSMap>("Coord");

  // Register Lua functions
  LuaContext::RegisterLuaFunc("map_generator.lua");
};
