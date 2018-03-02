#include "cpp_interface.h"


void AttackRuleBook::Init() {
    init("attack_rule_book.lua");
}

bool AttackRuleBook::CanAttack(UnitType unit, UnitType target) {
    bool ret = false;
    Invoke("attack_rule_book", "can_attack", &ret,
        static_cast<int>(unit), static_cast<int>(target));
    return ret;
}

void MoveRuleBook::Init() {
    init("move_rule_book.lua");
}

bool MoveRuleBook::CanMove(UnitType unit, Terrain cell) {
    bool ret = false;
    Invoke("move_rule_book", "can_move", &ret, unit, cell);
    return ret;
}

void reg_engine_cpp_interfaces() {
    AttackRuleBook::Init();
    MoveRuleBook::Init();
}
