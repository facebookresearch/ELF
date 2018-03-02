local map_parser = require 'map_parser'

rts_map_generator = {}

function rts_map_generator.generate_terrain(map, location_func, ty)
  local xs, ys = location_func()
  for i = 1, #xs do
    map:set_slot_type(ty, xs[i] - 1, ys[i] - 1, 0)
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

  parser = map_parser:new("two_rivers.map")
  map:init_map(parser:get_x_size(), parser:get_y_size(), 1)
  rts_map_generator.generate_terrain(map, function() return parser:get_sand_locations() end, Terrain.SAND)
  rts_map_generator.generate_terrain(map, function() return parser:get_grass_locations() end, Terrain.GRASS)
  rts_map_generator.generate_terrain(map, function() return parser:get_rock_locations() end, Terrain.ROCK)
  rts_map_generator.generate_terrain(map, function() return parser:get_water_locations() end, Terrain.WATER)
  map:reset_intermediates()
end


function is_in(map, x, y)
  local X = map:get_x_size()
  local Y = map:get_y_size()
  if x >= 0 and x < X and y >= 0 and y < Y then
    return true
  else
    return false
  end
end

function sign(x)
  if x < 0 then
    return -1
  elseif x > 1 then
    return 1
  else
    return 0
  end
end

function add_line(map, sx, sy, tx, ty, terrain)
  print(sx, sy, tx, ty)
  local X = map:get_x_size()
  local Y = map:get_y_size()
  local dx = math.max(1, math.abs(sx - tx))
  local dy = math.max(1, math.abs(sy - ty))
  local d = math.max(dx, dy)
  local sigx = sign(tx - sx)
  local sigy = sign(ty - sy)

  for i = 0, d do
    local x = sigx * math.floor(i / d * dx) + sx
    local y = sigy * math.floor(i / d * dy) + sy
    map:set_slot_type(terrain, x, y, 0)
    -- add margin
    if dy > dx then
      map:set_slot_type(terrain, x - 1, y, 0)
      --map:set_slot_type(terrain, x + 1, y, 0)
    else
      map:set_slot_type(terrain, x, y - 1, 0)
      --map:set_slot_type(terrain, x, y + 1, 0)
    end
  end
end

function rts_map_generator.generate_water_hor(map)
  local N = 5
  local delta = math.floor(40 / N)
  local last_y = math.random(1, N - 1)
  for xi = 1, N do
    local ly = math.max(last_y - 2, 1)
    local ry = math.min(last_y + 2, N - 1)
    local y = math.random(ly, ry)
    add_line(map, (xi - 1) * delta, last_y * delta, xi * delta, y * delta, Terrain.WATER)
    last_y = y
  end
end

function rts_map_generator.generate_water_ver(map)
  local N = 5
  local delta = math.floor(40 / N)
  local last_x = math.random(1, N - 1)
  print(last_x)
  for yi = 1, N do
    local lx = math.max(last_x - 2, 1)
    local rx = math.min(last_x + 2, N - 1)
    print(lx, rx)
    local x = math.random(lx, rx)
    print(x)
    add_line(map, last_x * delta, (yi - 1) * delta, x * delta, yi * delta, Terrain.WATER)
    last_x = x
  end
end


function rts_map_generator.generate_water(map)
  local water_type = math.random(0, 1)
  if water_type == 0 then
    rts_map_generator.generate_water_hor(map)
  elseif water_type == 1 then
    rts_map_generator.generate_water_ver(map)
  end
end


function rts_map_generator.generate_random(map, num_players, seed)
  math.randomseed(seed)
  map:clear_map()
  local X = 40
  local Y = 40

  map:init_map(X, Y, 1)
  rts_map_generator.generate_water(map)


  map:reset_intermediates()
end
