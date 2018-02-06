#pragma once

#include "elf/lua/interface.h"

#include "engine/map.h"
#include "engine/gamedef.h"


struct RTSMapGenerator : public CppClassInterface<RTSMapGenerator> {
    static void Init();

    static void Generate(RTSMap& map, int num_players, int seed);
};

struct RTSUnitGenerator : public CppClassInterface<RTSUnitGenerator> {
    static void Init();

    static void Generate(RTSMap& map, int seed);
};

struct RTSUnitFactory : public CppClassInterface<RTSUnitFactory> {
    static void Init();

    static UnitTemplate InitResource();

    static UnitTemplate InitWorker();

    static UnitTemplate InitMeleeAttacker();

    static UnitTemplate InitRangeAttacker();

    static UnitTemplate InitBarracks();

    static UnitTemplate InitBase();
};

void reg_cpp_interfaces();
