rts_unit_factory = {}

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
    0, 1000, 1000, 0, 0, 0, 0, {0, 0, 0, 0}, {}, UnitAttr.ATTR_INVULNERABLE)
  return ut
end


function rts_unit_factory.init_worker()
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK, CmdType.BUILD, CmdType.GATHER}
  local ut = __create_unit_template(
    50, 50, 0, 0.1, 2, 1, 3, {0, 10, 40, 40}, allowed_cmds, UnitAttr.ATTR_NORMAL)
  return ut
end

function rts_unit_factory.init_melee_attacker()
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local ut = __create_unit_template(
    100, 100, 1, 0.1, 15, 1, 3, {0, 15, 0, 0}, allowed_cmds, UnitAttr.ATTR_NORMAL)
  return ut
end

function rts_unit_factory.init_range_attacker()
  local allowed_cmds = {CmdType.MOVE, CmdType.ATTACK}
  local ut = __create_unit_template(
    100, 50, 0, 0.2, 10, 5, 5, {0, 10, 0, 0}, allowed_cmds, UnitAttr.ATTR_NORMAL)
  return ut
end

function rts_unit_factory.init_barracks()
  local allowed_cmds = {CmdType.BUILD}
  local ut = __create_unit_template(
    200, 200, 1, 0.0, 0, 0, 5, {0, 0, 0, 50}, allowed_cmds, UnitAttr.ATTR_NORMAL)
  return ut
end

function rts_unit_factory.init_base()
  local allowed_cmds = {CmdType.BUILD}
  local ut = __create_unit_template(
    500, 500, 2, 0.0, 0, 0, 5, {0, 0, 0, 50}, allowed_cmds, UnitAttr.ATTR_NORMAL)
  return ut
end
