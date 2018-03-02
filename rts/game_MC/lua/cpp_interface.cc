#include "cpp_interface.h"

void RTSMapGenerator::Init() {
    init("map_generator.lua");
}

void RTSMapGenerator::Generate(RTSMap& map, int num_players, int seed) {
    Invoke<void>("rts_map_generator", "generate", map, num_players, seed);
}


void RTSUnitGenerator::Init() {
    init("unit_generator.lua");
}

void RTSUnitGenerator::Generate(RTSMap* map, int num_players, int seed, CmdReceiver* cmd_receiver) {
    auto proxy = StateProxy{map, cmd_receiver};
    Invoke<void>("rts_unit_generator", "generate", proxy, num_players, seed);
}


void RTSUnitFactory::Init() {
    init("unit_factory.lua");
}

UnitTemplate RTSUnitFactory::InitResource() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_resource", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitPeasant() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_peasant", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitSwordman() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_swordman", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitSpearman() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_spearman", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitCavalry() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_cavalry", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitDragon() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_dragon", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitArcher() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_archer", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitCatapult() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_catapult", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitBarrack() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_barrack", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitBlacksmith() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_blacksmith", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitStables() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_stables", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitWorkshop() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_workshop", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitGuardTower() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_guard_tower", &ret);
    return ret;
}

UnitTemplate RTSUnitFactory::InitTownHall() {
    UnitTemplate ret;
    Invoke("rts_unit_factory", "init_town_hall", &ret);
    return ret;
}


void reg_cpp_interfaces() {
  RTSMapGenerator::Init();
  RTSUnitGenerator::Init();
  RTSUnitFactory::Init();
}
