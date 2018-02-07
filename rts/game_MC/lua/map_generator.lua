local map_parser = require 'map_parser'

rts_map_generator = {}

function rts_map_generator.generate_water(map, parser)
  local xs, ys = parser:get_water_locations()
  for i = 1, #xs do
    map:set_slot_type(Terrain.WATER, xs[i], ys[i], 0)
  end
end

function rts_map_generator.generate_forest(map, parser)
  local xs, ys = parser:get_forest_locations()
  for i = 1, #xs do
    map:set_slot_type(Terrain.IMPASSABLE, xs[i], ys[i], 0)
  end
end


function rts_map_generator.generate_base(map)
  local x = math.random(0, map:get_x_size() - 1)
  local y = math.random(0, map:get_y_size() - 1)
  return x, y
end

function rts_map_generator.generate2(map, num_players, seed)
  math.randomseed(seed)
  map:clear_map()
  map:init_map(30, 30, 1)
  rts_map_generator.generate_forest(map, 3)
  rts_map_generator.generate_water(map, 3)
  local init_resource = 200
  for player_id = 0, num_players - 1 do
    local base_x, base_y = rts_map_generator.generate_base(map)
    local resource_x, resource_y = rts_map_generator.generate_base(map)
    map:add_player(player_id, base_x, base_y, resource_x, resource_y, init_resource)
  end
  map:reset_intermediates()
end

function rts_map_generator.generate(map, num_players, seed)
  math.randomseed(seed)
  map:clear_map()
  parser = map_parser:new("m1.map")
  map:init_map(parser:get_x_size(), parser:get_y_size(), 1)
  rts_map_generator.generate_forest(map, parser)
  rts_map_generator.generate_water(map, parser)
  map:reset_intermediates()
end
