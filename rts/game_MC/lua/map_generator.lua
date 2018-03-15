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
  local y = math.random(0, map:get_y_size() - 1) return x, y
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

function add_line(map, sx, sy, tx, ty, terrain, has_hole)
  local X = map:get_x_size()
  local Y = map:get_y_size()
  local dx = math.max(1, math.abs(sx - tx))
  local dy = math.max(1, math.abs(sy - ty))
  local d = math.max(dx, dy)
  local sigx = sign(tx - sx)
  local sigy = sign(ty - sy)
  local hole_size = math.random(2, 4)
  local hole_loc = math.random(0, d - hole_size)

  for i = 0, d do
    local x = sigx * math.floor(i / d * dx) + sx
    local y = sigy * math.floor(i / d * dy) + sy
    local put_hole = has_hole and i >= hole_loc and i - hole_loc < hole_size
    if not put_hole then
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
end

function gen_holes_mask(N)
  local n_holes = math.random(1, math.min(3, N))
  local holes_mask = {}
  for i = 1, N do
    holes_mask[i] = false
  end
  for i = 1, n_holes do
    local found = false
    while not found do
      local x = math.random(1, N)
      if holes_mask[x] == false then
        holes_mask[x] = true
        found = true
      end
    end
  end
  return holes_mask
end

function rts_map_generator.generate_water_hor(map)
  local N = 5
  local delta = math.floor(map:get_x_size() / N)
  local last_y = math.random(1, N - 1)
  local holes_mask = gen_holes_mask(N)
  for xi = 1, N do
    local ly = math.max(last_y - 2, 1)
    local ry = math.min(last_y + 2, N - 1)
    local y = math.random(ly, ry)
    add_line(map, (xi - 1) * delta, last_y * delta, xi * delta, y * delta,
      Terrain.WATER, holes_mask[xi])
    last_y = y
  end
end

function rts_map_generator.generate_water_ver(map)
  local N = 5
  local delta = math.floor(map:get_x_size() / N)
  local last_x = math.random(1, N - 1)
  local holes_mask = gen_holes_mask(N)
  for yi = 1, N do
    local lx = math.max(last_x - 2, 1)
    local rx = math.min(last_x + 2, N - 1)
    local x = math.random(lx, rx)
    add_line(map, last_x * delta, (yi - 1) * delta, x * delta, yi * delta,
      Terrain.WATER, holes_mask[yi])
    last_x = x
  end
end


function rts_map_generator.generate_water(map)
  rts_map_generator.generate_water_hor(map)
  rts_map_generator.generate_water_ver(map)
end

function rts_map_generator.generate_soil_and_sand(map)
  local X = map:get_x_size()
  local Y = map:get_y_size()
  local dx = {-1, -1, 1, 1}
  local dy = {-1, 1, -1, 1}
  for x = 0, X - 1 do
    for y = 0, Y -1 do
      local any = false
      if map:get_slot_type(x, y, 0) == Terrain.GRASS then
        for i=1, #dx do
          local nx = x + dx[i]
          local ny = y + dy[i]
          if nx >= 0 and nx < X and ny >= 0 and ny < Y and map:get_slot_type(nx, ny, 0) == Terrain.WATER then
            local p = math.random(0, 999) / 1000
            if p < 0.1 then
              map:set_slot_type(Terrain.SOIL, x, y, 0)
              any = true
            elseif p < 0.3 then
              map:set_slot_type(Terrain.SAND, x, y, 0)
              any = true
            end
            break
          end
        end
      end
      if not any then
          local p = math.random(0, 999) / 1000
          if p < 0.01 then
              map:set_slot_type(Terrain.SAND, x, y, 0)
          end
      end
    end
  end
end

function rts_map_generator.generate_rock(map)
  local X = map:get_x_size()
  local Y = map:get_y_size()
  for x = 0, X - 1 do
    for y = 0, Y - 1 do
      if map:get_slot_type(x, y, 0) == Terrain.GRASS then
        local p = math.random(0, 999) / 1000
        if p < 0.03 then
          map:set_slot_type(Terrain.ROCK, x, y, 0)
        end
      end
    end
  end
end


function rts_map_generator.generate_random(map, num_players, seed)
  math.randomseed(seed)
  map:clear_map()
  local X = 40
  local Y = 30

  map:init_map(X, Y, 1)
  rts_map_generator.generate_water(map)
  --rts_map_generator.generate_soil_and_sand(map)
  rts_map_generator.generate_rock(map)


  map:reset_intermediates()
end
