attack_rule_book = {}


function attack_rule_book.can_attack(unit, target)
  if unit == UnitType.DRAGON then
    if target == UnitType.RANGE_ATTACKER then
      return false
    end
    return true
  end
  if unit == UnitType.MELEE_ATTACKER then
    if target == UnitType.DRAGON then
      return false
    end
    return true
  end
  if unit == UnitType.RANGE_ATTACKER then
    if target == UnitType.MELEE_ATTACKER then
      return false
    end
    return true
  end
  return true
end
