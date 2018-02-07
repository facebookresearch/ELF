#pragma once

#include "elf/lua/interface.h"

#include "engine/common.h"

struct AttackRuleBook : public CppClassInterface<AttackRuleBook> {
    static void Init();

    static bool CanAttack(UnitType unit, UnitType target);
};

struct MoveRuleBook : public CppClassInterface<MoveRuleBook> {
    static void Init();

    static bool CanMove(UnitType unit, Terrain cell);
};

void reg_engine_cpp_interfaces();
