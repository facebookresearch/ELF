#pragma once

#include "elf/lua/interface.h"

#include "engine/lua/utils.h"
#include "engine/map.h"
#include "engine/gamedef.h"
#include "engine/cmd_receiver.h"


struct RTSMapGenerator : public CppClassInterface<RTSMapGenerator> {
    static void Init();

    static void Generate(RTSMap& map, int num_players, int seed);

    static void GenerateRandom(RTSMap& map, int num_players, int seed);
};

struct RTSUnitGenerator : public CppClassInterface<RTSUnitGenerator> {
    static void Init();

    static void Generate(RTSMap* map, int num_players, int seed, CmdReceiver* cmd_receiver);

    static void GenerateRandom(RTSMap* map, int num_players, int seed, CmdReceiver* cmd_receiver);
};

struct RTSUnitFactory : public CppClassInterface<RTSUnitFactory> {
    static void Init();

    static UnitTemplate InitResource();

    static UnitTemplate InitPeasant();

    static UnitTemplate InitSwordman();

    static UnitTemplate InitSpearman();

    static UnitTemplate InitCavalry();

    static UnitTemplate InitArcher();

    static UnitTemplate InitDragon();

    static UnitTemplate InitCatapult();

    static UnitTemplate InitBarrack();

    static UnitTemplate InitBlacksmith();

    static UnitTemplate InitStables();

    static UnitTemplate InitWorkshop();

    static UnitTemplate InitGuardTower();

    static UnitTemplate InitTownHall();
};

void reg_cpp_interfaces();
