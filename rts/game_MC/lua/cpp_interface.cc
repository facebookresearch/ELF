#include "cpp_interface.h"

void RTSMapGenerator::Init() {
    init("map_generator.lua");
}

void RTSMapGenerator::Generate(RTSMap& map, int num_players, int seed) {
    Invoke<void>("rts_map_generator", "generate", map, num_players, seed);
}

void RTSMapGenerator::GenerateRandom(RTSMap& map, int num_players, int seed) {
    Invoke<void>("rts_map_generator", "generate_random", map, num_players, seed);
}


void RTSUnitGenerator::Init() {
    init("unit_generator.lua");
}

void RTSUnitGenerator::Generate(RTSMap* map, int num_players, int seed, CmdReceiver* cmd_receiver) {
    auto proxy = StateProxy{map, cmd_receiver};
    Invoke<void>("rts_unit_generator", "generate", proxy, num_players, seed);
}

void RTSUnitGenerator::GenerateRandom(RTSMap* map, int num_players, int seed, CmdReceiver* cmd_receiver) {
    auto proxy = StateProxy{map, cmd_receiver};
    Invoke<void>("rts_unit_generator", "generate_random", proxy, num_players, seed);
}

void RTSUnitFactory::Init() {
    init("unit_factory.lua");
}

UnitTemplate RTSUnitFactory::InitResource() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_resource", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitWorker() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_worker", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitSoldier() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_soldier", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitTruck() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_truck", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitTank() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_tank", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitFlight() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_flight", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitCannon() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_cannon", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitBarrack() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_barrack", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitFactory() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_factory", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitHangar() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_hangar", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitWorkshop() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_workshop", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitDefenseTower() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_defense_tower", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitBase() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_base", &ret);
    return ret;
}


void reg_cpp_interfaces() {
  RTSMapGenerator::Init();
  RTSUnitGenerator::Init();
  RTSUnitFactory::Init();
}
