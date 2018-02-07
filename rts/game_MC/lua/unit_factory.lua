rts_unit_factory = {}

--[[
--Cooldown is specified by a vector of 4 numbers in the following order:
--[CD_MOVE, CD_ATTACK, CD_GATHER, CD_BUILD]
]]

function __create_unit_template(cost, hp, defense, speed, att, att_r, vis_r,
  cds, allowed_cmds, attr)

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
  return ut
end

function rts_unit_factory.init_resource()
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
    --[[attr]]UnitAttr.ATTR_INVULNERABLE
  )
  return ut
end


function rts_unit_factory.init_worker()
  local allowed_cmds = {CmdType.MOVE, CmdType.BUILD, CmdType.GATHER}
  local ut = __create_unit_template(
    --[[cost]]50,
    --[[hp]]50,
    --[[defense]]0,
    --[[speed]]0.1,
    --[[att]]2,
    --[[att_r]]1,
    --[[vis_r]]3,
    --[[cds]]{0, 10, 40, 40},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL
  )
  return ut
end

function rts_unit_factory.init_melee_attacker()
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local ut = __create_unit_template(
    --[[cost]]100,
    --[[hp]]100,
    --[[defense]]1,
    --[[speed]]0.1,
    --[[att]]15,
    --[[att_r]]1,
    --[[vis_r]]3,
    --[[cds]]{0, 15, 0, 0},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL
  )
  return ut
end

function rts_unit_factory.init_range_attacker()
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local ut = __create_unit_template(
    --[[cost]]100,
    --[[hp]]50,
    --[[defense]]0,
    --[[speed]]0.2,
    --[[att]]10,
    --[[att_r]]5,
    --[[vis_r]]5,
    --[[cds]]{0, 10, 0, 0},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL
  )
  return ut
end

function rts_unit_factory.init_flight()
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local ut = __create_unit_template(
    --[[cost]]200,
    --[[hp]]200,
    --[[defense]]3,
    --[[speed]]0.4,
    --[[att]]30,
    --[[att_r]]10,
    --[[vis_r]]10,
    --[[cds]]{0, 10, 0, 0},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL
  )
  return ut
end

function rts_unit_factory.init_barracks()
  local allowed_cmds = {CmdType.BUILD}
  local ut = __create_unit_template(
    --[[cost]]200,
    --[[hp]]200,
    --[[defense]]1,
    --[[speed]]0.0,
    --[[att]]0,
    --[[att_r]]0,
    --[[vis_r]]5,
    --[[cds]]{0, 0, 0, 50},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL
  )
  return ut
end

function rts_unit_factory.init_base()
  local allowed_cmds = {CmdType.BUILD}
  local ut = __create_unit_template(
    --[[cost]]500,
    --[[hp]]500,
    --[[defense]]2,
    --[[speed]]0.0,
    --[[att]]0,
    --[[att_r]]0,
    --[[vis_r]]5,
    --[[cds]]{0, 0, 0, 50},
    --[[allowed_cmds]]allowed_cmds,
    --[[attr]]UnitAttr.ATTR_NORMAL
  )
  return ut
end
