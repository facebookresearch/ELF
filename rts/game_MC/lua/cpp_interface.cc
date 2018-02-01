#include "cpp_interface.h"

void RTSMapGenerator::Init() {
  init("map_generator.lua");
}

void RTSMapGenerator::Generate(RTSMap& map, int num_players, int init_resource) {
    Invoke<void>("rts_map_generator", "generate", map, num_players, init_resource);
}


void reg_cpp_interfaces() {
  RTSMapGenerator::Init();
}
