local map_parser = require 'map_parser'

rts_unit_generator = {}

function __make_p(x, y)
  local p = PointF.new()
  p:set_int_xy(x, y)
  return p
end


function rts_unit_generator.generate2(proxy, num_players, seed)
  math.randomseed(seed)
  local X = proxy:get_x_size()
  local Y = proxy:get_y_size()
  print(X)
  print(Y)
  local player_id = 0
  local enemy_id = 1

  proxy:send_cmd_create(UnitType.RESOURCE, __make_p(2, 1), player_id, 0)
  proxy:send_cmd_create(UnitType.WORKER, __make_p(4, 4), player_id, 0)
  proxy:send_cmd_create(UnitType.WORKER, __make_p(5, 5), player_id, 0)
  proxy:send_cmd_create(UnitType.WORKER, __make_p(6, 7), player_id, 0)
  proxy:send_cmd_create(UnitType.FLIGHT, __make_p(8, 9), player_id, 0)
  proxy:send_cmd_create(UnitType.BASE, __make_p(7, 2), player_id, 0)
  proxy:send_cmd_change_player_resource(player_id, 100)

  proxy:send_cmd_create(UnitType.RESOURCE, __make_p(14, 18), enemy_id, 0)
  proxy:send_cmd_create(UnitType.BASE, __make_p(18, 16), enemy_id, 0)
  proxy:send_cmd_create(UnitType.RANGE_ATTACKER, __make_p(20, 21), enemy_id, 0)
  proxy:send_cmd_change_player_resource(enemy_id, 100)
end

local unit_type_to_cell = {}
unit_type_to_cell[UnitType.RESOURCE] = {"C", "c"}
unit_type_to_cell[UnitType.WORKER] = {"W", "w"}
unit_type_to_cell[UnitType.FLIGHT] = {"F", "f"}
unit_type_to_cell[UnitType.MELEE_ATTACKER] = {"M", "m"}
unit_type_to_cell[UnitType.RANGE_ATTACKER] = {"R", "r"}
unit_type_to_cell[UnitType.BASE] = {"B", "b"}
unit_type_to_cell[UnitType.BARRACKS] = {"A", "a"}


function rts_unit_generator.generate_unit(proxy, parser, player_id, unit_type)
  local ty = unit_type_to_cell[unit_type][player_id + 1]
  local xs, ys = parser:get_locations(ty)
  for i = 1, #xs do
    proxy:send_cmd_create(unit_type, __make_p(xs[i], ys[i]), player_id, 0)
  end
end


function rts_unit_generator.generate(proxy, num_players, seed)
  math.randomseed(seed)
  local player_id = 0
  local enemy_id = 1

  parser = map_parser:new("m1.map")

  -- your units
  rts_unit_generator.generate_unit(proxy, parser, 0, UnitType.RESOURCE)
  rts_unit_generator.generate_unit(proxy, parser, 0, UnitType.WORKER)
  rts_unit_generator.generate_unit(proxy, parser, 0, UnitType.MELEE_ATTACKER)
  rts_unit_generator.generate_unit(proxy, parser, 0, UnitType.RANGE_ATTACKER)
  rts_unit_generator.generate_unit(proxy, parser, 0, UnitType.BASE)
  rts_unit_generator.generate_unit(proxy, parser, 0, UnitType.FLIGHT)
  rts_unit_generator.generate_unit(proxy, parser, 0, UnitType.BARRACKS)

  -- enemy units
  rts_unit_generator.generate_unit(proxy, parser, 1, UnitType.RESOURCE)
  rts_unit_generator.generate_unit(proxy, parser, 1, UnitType.WORKER)
  rts_unit_generator.generate_unit(proxy, parser, 1, UnitType.MELEE_ATTACKER)
  rts_unit_generator.generate_unit(proxy, parser, 1, UnitType.RANGE_ATTACKER)
  rts_unit_generator.generate_unit(proxy, parser, 1, UnitType.BASE)
  rts_unit_generator.generate_unit(proxy, parser, 1, UnitType.FLIGHT)
  rts_unit_generator.generate_unit(proxy, parser, 1, UnitType.BARRACKS)

  -- change resources
  proxy:send_cmd_change_player_resource(0, 100)
  proxy:send_cmd_change_player_resource(1, 100)
end
