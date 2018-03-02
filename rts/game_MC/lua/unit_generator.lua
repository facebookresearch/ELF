local map_parser = require 'map_parser'

rts_unit_generator = {}

function __make_p(x, y)
  local p = PointF.new()
  p:set_int_xy(x, y)
  return p
end


local unit_type_to_cell = {}
unit_type_to_cell[UnitType.RESOURCE] = "X"
unit_type_to_cell[UnitType.PEASANT] = "W"
unit_type_to_cell[UnitType.SWORDMAN] = "S"
unit_type_to_cell[UnitType.SPEARMAN] = "R"
unit_type_to_cell[UnitType.CAVALRY] = "T"
unit_type_to_cell[UnitType.ARCHER] = "C"
unit_type_to_cell[UnitType.DRAGON] = "F"
unit_type_to_cell[UnitType.BARRACK] = "A"
unit_type_to_cell[UnitType.BLACKSMITH] = "O"
unit_type_to_cell[UnitType.STABLES] = "H"
unit_type_to_cell[UnitType.WORKSHOP] = "K"
unit_type_to_cell[UnitType.GUARD_TOWER] = "D"
unit_type_to_cell[UnitType.TOWN_HALL] = "B"


function rts_unit_generator.generate_unit(proxy, parser, player_id, unit_type)
  local ty = unit_type_to_cell[unit_type]
  -- if enemy, lower case
  if player_id == 1 then
    ty = ty:lower()
  end
  local xs, ys = parser:get_locations(ty)
  for i = 1, #xs do
    proxy:send_cmd_create(unit_type, __make_p(xs[i] - 1, ys[i] - 1), player_id, 0)
  end
end


function rts_unit_generator.generate(proxy, num_players, seed)
  math.randomseed(seed)
  local player_id = 0
  local enemy_id = 1

  parser = map_parser:new("two_rivers.map")

  -- your units
  for k, v in pairs(unit_type_to_cell) do
    rts_unit_generator.generate_unit(proxy, parser, 0, k)
  end

  -- enemy units
  for k, v in pairs(unit_type_to_cell) do
    rts_unit_generator.generate_unit(proxy, parser, 1, k)
  end

  -- change resources
  proxy:send_cmd_change_player_resource(0, 1500)
  proxy:send_cmd_change_player_resource(1, 100)
end
