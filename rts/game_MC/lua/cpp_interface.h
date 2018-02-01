#pragma once

#include "elf/lua/interface.h"

#include "engine/map.h"


struct RTSMapGenerator : public CppInterface<RTSMapGenerator> {
    static void Init();

    static void Generate(RTSMap& map, int num_players, int init_resource);
};


void reg_cpp_interfaces();
