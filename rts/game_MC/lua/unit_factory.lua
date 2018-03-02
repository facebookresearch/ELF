rts_unit_factory = {}

--[[
--Cooldown is specified by a vector of 4 numbers in the following order:
--[CD_MOVE, CD_ATTACK, CD_GATHER, CD_BUILD]
]]

function __create_build_skill(unit_type, hotkey)
  local skill = BuildSkill.new()
  skill:set_unit_type(unit_type)
  skill:set_hotkey(hotkey)
  return skill
end

function __create_unit_template(cost, hp, defense, speed, att, att_r, vis_r,
  cds, allowed_cmds, attr, attack_multiplier, cant_move_over, build_from, build_skills)

  local up = UnitProperty.new()
  up:set_hp(hp)
  up:set_max_hp(hp)
  up:set_speed(speed)
  up:set_def(defense)
  up:set_attr(attr)
  up:set_att(att)
  up:set_att_r(att_r)
  up:set_vis_r(vis_r)

  for i = 1, CDType.NUM_COOLDOWN do
    local cd = Cooldown.new()
    cd:set(cds[i])
    up:set_cooldown(i - 1, cd)
  end

  local ut = UnitTemplate.new()
  ut:set_property(up)
  ut:set_build_cost(cost)

  for i = 1, #allowed_cmds do
    ut:add_allowed_cmd(allowed_cmds[i])
  end
  for i = 1, #attack_multiplier do
    local ty = attack_multiplier[i][1];
    local mult = attack_multiplier[i][2];
    ut:set_attack_multiplier(ty, mult);
  end
  for i = 1, #cant_move_over do
    ut:add_cant_move_over(cant_move_over[i])
  end
  for i = 1, #build_skills do
    ut:add_build_skill(build_skills[i])
  end
  ut:set_build_from(build_from)
  return ut
end

function rts_unit_factory.init_resource()
  -- stationary, high hp, invisible
  local ut = __create_unit_template(
    --[[cost]]0,
    --[[hp]]1000,
    --[[defense]]1000,
    --[[speed]]0,
    --[[att]]0,
    --[[att_r]]0,
    --[[vis_r]]0,
    --[[cds]]{0, 0, 0, 0},
    --[[allowed_cmds]]{},
    --[[attr]]UnitAttr.ATTR_INVULNERABLE,
    --[[can_attack]]{},
    --[[cant_move_over]]{},
    --[[build_from]]-1,
    --[[build_skills]]{}
  )
  return ut
end


function rts_unit_factory.init_peasant()
  -- worker unit, can't attack, only can build
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK, CmdType.BUILD, CmdType.GATHER}
  local attack_multiplier = {
  }
  local cant_move_over = {Terrain.ROCK, Terrain.WATER}
  local build_skills = {
    __create_build_skill(UnitType.TOWN_HALL, "h"),
    __create_build_skill(UnitType.BARRACK, "b"),
    __create_build_skill(UnitType.BLACKSMITH, "s"),
    __create_build_skill(UnitType.STABLES, "l"),
    __create_build_skill(UnitType.WORKSHOP, "w"),
    __create_build_skill(UnitType.GUARD_TOWER, "t")
  }
  local ut = __create_unit_template(
    --[[cost]]50,
    --[[hp]]30,
    --[[defense]]0,
    --[[speed]]0.05,
    --[[att]]1,
    --[[att_r]]1,
    --[[vis_r]]3,
    --[[cds]]{0, 40, 40, 150},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.TOWN_HALL,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_swordman()
  -- very cheap and weak unit, medium speed
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.PEASANT, 1.0},
    {UnitType.CAVALRY, 1.0},
    {UnitType.ARCHER, 1.0},
    {UnitType.CATAPULT, 1.0},
    {UnitType.BARRACK, 1.0},
    {UnitType.BLACKSMITH, 1.0},
    {UnitType.STABLES, 1.0},
    {UnitType.WORKSHOP, 1.0},
    {UnitType.GUARD_TOWER, 1.0},
    {UnitType.TOWN_HALL, 1.0}
  }

  local cant_move_over = {Terrain.ROCK, Terrain.WATER}
  local build_skills = {}
  local ut = __create_unit_template(
    --[[cost]]70,
    --[[hp]]100,
    --[[defense]]1,
    --[[speed]]0.05,
    --[[att]]7,
    --[[att_r]]2,
    --[[vis_r]]5,
    --[[cds]]{0, 10, 0, 0},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.BLACKSMITH,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_spearman()
  -- fast but weak unit
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.PEASANT, 1.0},
    {UnitType.SWORDMAN, 1.0},
    {UnitType.ARCHER, 1.0},
    {UnitType.BARRACK, 1.0},
    {UnitType.CATAPULT, 1.0},
    {UnitType.BLACKSMITH, 1.0},
    {UnitType.STABLES, 1.0},
    {UnitType.WORKSHOP, 1.0},
    {UnitType.GUARD_TOWER, 1.0},
    {UnitType.TOWN_HALL, 1.0}
  }
  local cant_move_over = {Terrain.ROCK, Terrain.WATER}
  local build_skills = {}
  local ut = __create_unit_template(
    --[[cost]]70,
    --[[hp]]100,
    --[[defense]]1,
    --[[speed]]0.05,
    --[[att]]7,
    --[[att_r]]2,
    --[[vis_r]]5,
    --[[cds]]{0, 10, 0, 0},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.BARRACK,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_cavalry()
  -- medium slow, but more powerful unit
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.PEASANT, 1.0},
    {UnitType.SPEARMAN, 1.0},
    {UnitType.ARCHER, 1.0},
    {UnitType.CATAPULT, 1.0},
    {UnitType.BARRACK, 1.0},
    {UnitType.BLACKSMITH, 1.0},
    {UnitType.STABLES, 1.0},
    {UnitType.WORKSHOP, 1.0},
    {UnitType.GUARD_TOWER, 1.0},
    {UnitType.TOWN_HALL, 1.0}
  }
  local cant_move_over = {Terrain.ROCK, Terrain.WATER}
  local build_skills = {}
  local ut = __create_unit_template(
    --[[cost]]70,
    --[[hp]]100,
    --[[defense]]1,
    --[[speed]]0.05,
    --[[att]]7,
    --[[att_r]]2,
    --[[vis_r]]5,
    --[[cds]]{0, 10, 0, 0},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.STABLES,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_archer()
  -- slow unit, but  powerful, and weak
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.DRAGON, 2.0}
  }

  local cant_move_over = {Terrain.ROCK, Terrain.WATER}
  local build_skills = {}
  local ut = __create_unit_template(
    --[[cost]]500,
    --[[hp]]150,
    --[[defense]]0,
    --[[speed]]0.05,
    --[[att]]100,
    --[[att_r]]7,
    --[[vis_r]]8,
    --[[cds]]{0, 30, 0, 0},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.WORKSHOP,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_dragon()
  -- quick, good for scouting
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.PEASANT, 1.0},
    {UnitType.SWORDMAN, 1.0},
    {UnitType.CATAPULT, 1.0},
    {UnitType.SPEARMAN, 1.0},
    {UnitType.CAVALRY, 1.0}
  }
  local cant_move_over = {}
  local build_skills = {}
  local ut = __create_unit_template(
    --[[cost]]200,
    --[[hp]]100,
    --[[defense]]2,
    --[[speed]]0.2,
    --[[att]]8,
    --[[att_r]]3,
    --[[vis_r]]6,
    --[[cds]]{0, 20, 0, 0},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.WORKSHOP,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_catapult()
  -- quick, good for scouting
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.PEASANT, 1.0},
    {UnitType.SWORDMAN, 1.0},
    {UnitType.CATAPULT, 1.0},
    {UnitType.SPEARMAN, 1.0},
    {UnitType.CAVALRY, 1.0}
  }
  local cant_move_over = {}
  local build_skills = {}
  local ut = __create_unit_template(
    --[[cost]]200,
    --[[hp]]100,
    --[[defense]]2,
    --[[speed]]0.2,
    --[[att]]8,
    --[[att_r]]7,
    --[[vis_r]]6,
    --[[cds]]{0, 20, 0, 0},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.WORKSHOP,
    --[[build_skills]]build_skills
  )
  return ut
end


function rts_unit_factory.init_barrack()
  local allowed_cmds = {CmdType.BUILD}
  local cant_move_over = {}
  local build_skills = {
    __create_build_skill(UnitType.SPEARMAN, "s")
  }
  local attack_multiplier = {}
  local ut = __create_unit_template(
    --[[cost]]200,
    --[[hp]]200,
    --[[defense]]5,
    --[[speed]]0.0,
    --[[att]]0,
    --[[att_r]]0,
    --[[vis_r]]5,
    --[[cds]]{0, 0, 0, 50},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.PEASANT,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_blacksmith()
  local allowed_cmds = {CmdType.BUILD}
  local attack_multiplier = {}
  local cant_move_over = {}
  local build_skills = {
    __create_build_skill(UnitType.SWORDMAN, "s")
  }
  local ut = __create_unit_template(
    --[[cost]]300,
    --[[hp]]300,
    --[[defense]]5,
    --[[speed]]0.0,
    --[[att]]0,
    --[[att_r]]0,
    --[[vis_r]]5,
    --[[cds]]{0, 0, 0, 70},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.PEASANT,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_stables()
  local allowed_cmds = {CmdType.BUILD}
  local attack_multiplier = {}
  local cant_move_over = {}
  local build_skills = {
    __create_build_skill(UnitType.CAVALRY, "c")
  }
  local ut = __create_unit_template(
    --[[cost]]250,
    --[[hp]]250,
    --[[defense]]5,
    --[[speed]]0.0,
    --[[att]]0,
    --[[att_r]]0,
    --[[vis_r]]5,
    --[[cds]]{0, 0, 0, 70},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.PEASANT,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_workshop()
  local allowed_cmds = {CmdType.BUILD}
  local attack_multiplier = {}
  local cant_move_over = {}
  local build_skills = {
    __create_build_skill(UnitType.ARCHER, "a"),
    __create_build_skill(UnitType.DRAGON, "d"),
    __create_build_skill(UnitType.CATAPULT, "c"),
  }
  local ut = __create_unit_template(
    --[[cost]]250,
    --[[hp]]250,
    --[[defense]]5,
    --[[speed]]0.0,
    --[[att]]0,
    --[[att_r]]0,
    --[[vis_r]]5,
    --[[cds]]{0, 0, 0, 70},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.PEASANT,
    --[[build_skills]]build_skills
  )
  return ut
end


function rts_unit_factory.init_guard_tower()
  local allowed_cmds = {CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.PEASANT, 1.0},
    {UnitType.SWORDMAN, 1.0},
    {UnitType.SPEARMAN, 1.0},
    {UnitType.CAVALRY, 1.0},
    {UnitType.ARCHER, 1.0},
    {UnitType.DRAGON, 1.0},
    {UnitType.CATAPULT, 1.0},
    {UnitType.BARRACK, 1.0},
    {UnitType.BLACKSMITH, 1.0},
    {UnitType.STABLES, 1.0},
    {UnitType.WORKSHOP, 1.0},
    {UnitType.GUARD_TOWER, 1.0},
    {UnitType.TOWN_HALL, 1.0}
  }
  local cant_move_over = {Terrain.ROCK, Terrain.WATER}
  local build_skills = {}
  local ut = __create_unit_template(
    --[[cost]]150,
    --[[hp]]150,
    --[[defense]]5,
    --[[speed]]0.0,
    --[[att]]0,
    --[[att_r]]5,
    --[[vis_r]]5,
    --[[cds]]{0, 50, 0, 0},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.PEASANT,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_town_hall()
  local allowed_cmds = {CmdType.BUILD}
  local attack_multiplier = {}
  local cant_move_over = {}
  local build_skills = {
    __create_build_skill(UnitType.PEASANT, "p"),
  }
  local ut = __create_unit_template(
    --[[cost]]500,
    --[[hp]]500,
    --[[defense]]5,
    --[[speed]]0.0,
    --[[att]]0,
    --[[att_r]]0,
    --[[vis_r]]5,
    --[[cds]]{0, 0, 0, 50},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.PEASANT,
    --[[build_skills]]build_skills
  )
  return ut
end
