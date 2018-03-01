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


function rts_unit_factory.init_worker()
  -- worker unit, can't attack, only can build
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK, CmdType.BUILD, CmdType.GATHER}
  local attack_multiplier = {
  }
  local cant_move_over = {Terrain.ROCK, Terrain.WATER}
  local build_skills = {
    __create_build_skill(UnitType.BASE, "b"),
    __create_build_skill(UnitType.BARRACK, "r"),
    __create_build_skill(UnitType.FACTORY, "f"),
    __create_build_skill(UnitType.HANGAR, "h"),
    __create_build_skill(UnitType.WORKSHOP, "w"),
    __create_build_skill(UnitType.DEFENSE_TOWER, "t")
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
    --[[build_from]]UnitType.BASE,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_soldier()
  -- very cheap and weak unit, medium speed
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.WORKER, 1.0},
    {UnitType.TANK, 1.0},
    {UnitType.CANNON, 1.0},
    {UnitType.BARRACK, 1.0},
    {UnitType.FACTORY, 1.0},
    {UnitType.HANGAR, 1.0},
    {UnitType.WORKSHOP, 1.0},
    {UnitType.DEFENSE_TOWER, 1.0},
    {UnitType.BASE, 1.0}
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

function rts_unit_factory.init_truck()
  -- fast but weak unit
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.WORKER, 1.0},
    {UnitType.SOLDIER, 1.0},
    {UnitType.CANNON, 1.0},
    {UnitType.BARRACK, 1.0},
    {UnitType.FACTORY, 1.0},
    {UnitType.HANGAR, 1.0},
    {UnitType.WORKSHOP, 1.0},
    {UnitType.DEFENSE_TOWER, 1.0},
    {UnitType.BASE, 1.0}
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
    --[[vis_r]]3,
    --[[cds]]{0, 10, 0, 0},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.WORKSHOP,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_tank()
  -- medium slow, but more powerful unit
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.WORKER, 1.0},
    {UnitType.TRUCK, 1.0},
    {UnitType.CANNON, 1.0},
    {UnitType.BARRACK, 1.0},
    {UnitType.FACTORY, 1.0},
    {UnitType.HANGAR, 1.0},
    {UnitType.WORKSHOP, 1.0},
    {UnitType.DEFENSE_TOWER, 1.0},
    {UnitType.BASE, 1.0}
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
    --[[vis_r]]3,
    --[[cds]]{0, 10, 0, 0},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL,
    --[[attack_multiplier]]attack_multiplier,
    --[[cant_move_over]]cant_move_over,
    --[[build_from]]UnitType.FACTORY,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_cannon()
  -- slow unit, but  powerful, and weak
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.FLIGHT, 2.0}
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
    --[[build_from]]UnitType.HANGAR,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_flight()
  -- quick, good for scouting
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.WORKER, 1.0},
    {UnitType.SOLDIER, 1.0},
    {UnitType.TRUCK, 1.0},
    {UnitType.TANK, 1.0}
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
    --[[build_from]]UnitType.HANGAR,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_barrack()
  local allowed_cmds = {CmdType.BUILD}
  local cant_move_over = {}
  local build_skills = {
    __create_build_skill(UnitType.SOLDIER, "s"),
    __create_build_skill(UnitType.TRUCK, "t")
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
    --[[build_from]]UnitType.WORKER,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_factory()
  local allowed_cmds = {CmdType.BUILD}
  local attack_multiplier = {}
  local cant_move_over = {}
  local build_skills = {
    __create_build_skill(UnitType.TANK, "t")
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
    --[[build_from]]UnitType.WORKER,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_hangar()
  local allowed_cmds = {CmdType.BUILD}
  local attack_multiplier = {}
  local cant_move_over = {}
  local build_skills = {
    __create_build_skill(UnitType.FLIGHT, "f")
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
    --[[build_from]]UnitType.WORKER,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_workshop()
  local allowed_cmds = {CmdType.BUILD}
  local attack_multiplier = {}
  local cant_move_over = {}
  local build_skills = {
    __create_build_skill(UnitType.CANNON, "c"),
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
    --[[build_from]]UnitType.WORKER,
    --[[build_skills]]build_skills
  )
  return ut
end


function rts_unit_factory.init_defense_tower()
  local allowed_cmds = {CmdType.ATTACK}
  local attack_multiplier = {
    {UnitType.WORKER, 1.0},
    {UnitType.SOLDIER, 1.0},
    {UnitType.TRUCK, 1.0},
    {UnitType.TANK, 1.0},
    {UnitType.CANNON, 1.0},
    {UnitType.FLIGHT, 1.0},
    {UnitType.BARRACK, 1.0},
    {UnitType.FACTORY, 1.0},
    {UnitType.HANGAR, 1.0},
    {UnitType.WORKSHOP, 1.0},
    {UnitType.DEFENSE_TOWER, 1.0},
    {UnitType.BASE, 1.0}
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
    --[[build_from]]UnitType.WORKER,
    --[[build_skills]]build_skills
  )
  return ut
end

function rts_unit_factory.init_base()
  local allowed_cmds = {CmdType.BUILD}
  local attack_multiplier = {}
  local cant_move_over = {}
  local build_skills = {
    __create_build_skill(UnitType.WORKER, "w"),
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
    --[[build_from]]UnitType.WORKER,
    --[[build_skills]]build_skills
  )
  return ut
end
