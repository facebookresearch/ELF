#pragma once

#include "elf/lua/interface.h"

#include "engine/lua/utils.h"
#include "engine/map.h"
#include "engine/gamedef.h"
#include "engine/cmd_receiver.h"


struct RTSMapGenerator : public CppClassInterface<RTSMapGenerator> {
    static void Init();

    static void Generate(RTSMap& map, int num_players, int seed);
};

struct RTSUnitGenerator : public CppClassInterface<RTSUnitGenerator> {
    static void Init();

    static void Generate(RTSMap* map, int num_players, int seed, CmdReceiver* cmd_receiver);
};

struct RTSUnitFactory : public CppClassInterface<RTSUnitFactory> {
    static void Init();

    static UnitTemplate InitResource();

    static UnitTemplate InitWorker();

    static UnitTemplate InitMeleeAttacker();

    static UnitTemplate InitRangeAttacker();

    static UnitTemplate InitFlight();

    static UnitTemplate InitBomber();

    static UnitTemplate InitBarracks();

    static UnitTemplate InitFactory();

    static UnitTemplate InitBase();
};

void reg_cpp_interfaces();
