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
unit_type_to_cell[UnitType.CATAPULT] = "U"
unit_type_to_cell[UnitType.BARRACK] = "A"
unit_type_to_cell[UnitType.BLACKSMITH] = "O"
unit_type_to_cell[UnitType.STABLE] = "H"
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

function can_pass(proxy, x, y)
  local X = proxy:get_x_size()
  local Y = proxy:get_y_size()
  if not (x >= 0 and x < X and y >= 0 and y < Y) then
    return false
  end
  local ty = proxy:get_slot_type(x, y, 0)
  return not (ty == Terrain.WATER or ty == Terrain.ROCK)
end


function can_reach(proxy, sx, sy, tx, ty)
  local X = proxy:get_x_size()
  local Y = proxy:get_y_size()
  local dx = {-1, -1, 1, 1}
  local dy = {-1, 1, -1, 1}

  function dfs(x, y, tx, ty, visited)
    if x == tx and y == ty then
      return true
    end
    visited[x * Y + y] = true
    for i = 1, #dx do
      local nx = x + dx[i]
      local ny = y + dy[i]
      if nx >= 0 and nx < X and ny >= 0 and ny < Y and visited[nx * Y + ny] == nil then
        if can_pass(proxy, nx, ny) and dfs(nx, ny, tx, ty, visited) then
          return true
        end
      end
    end
    return false
  end
  return dfs(sx, sy, tx, ty, {})
end


function add_base_and_workers(proxy, x, y, player_id)
  local X = proxy:get_x_size()
  local Y = proxy:get_y_size()
  local n_workers = 3
  proxy:send_cmd_create(UnitType.TOWN_HALL, __make_p(x, y), player_id, 0)
  local taken = {}
  taken[x * Y + y] = true
  for i = 1, n_workers do
    local found = false
    local iter = 0
    while not found do
      iter = iter + 1
      if iter > 20 then
        break
      end
      local xx = x + math.random(-5, 5)
      local yy = y + math.random(-5, 5)
      if can_pass(proxy, xx, yy) and taken[xx * Y + yy] == nil then
        proxy:send_cmd_create(UnitType.PEASANT, __make_p(xx, yy), player_id, 0)
        taken[xx * Y + yy] = true
        found = true
      end
    end
  end
  local n_resource = 1
  for i = 1, n_resource do
    local found = false
    local iter = 0
    while not found do
      iter = iter + 1
      if iter > 20 then
        break
      end
      local xx = x + math.random(-3, 3)
      local yy = y + math.random(-3, 3)
      local dist = math.abs(xx - x) + math.abs(yy - y)
      if dist >= 4 and can_pass(proxy, xx, yy) and taken[xx * Y + yy] == nil then
        proxy:send_cmd_create(UnitType.RESOURCE, __make_p(xx, yy), player_id, 0)
        taken[xx * Y + yy] = true
        found = true
      end
    end
  end
end


function generate_bases(proxy)
  local X = proxy:get_x_size()
  local Y = proxy:get_y_size()
  local n_resource = 2
  local N = 5
  local dx = math.floor(X / N)
  local dy = math.floor(Y / N)
  local p = math.random(0, 999) / 1000
  local dir = p < 0.5

  local found = false
  local iter = 0
  while not found do
    iter = iter + 1
    local xp1 = math.random(0, dx - 1)
    local xp2 = math.random(0, dx - 1)
    local x1 = math.random(2, N - 1) + xp1 * N
    local y1 = math.random(2, N - 1)
    local x2 = math.random(2, N - 1) + xp2 * N
    local y2 = Y - 1 - math.random(2, N - 1)
    -- swap the coordinates
    if dir then
      x1, y1 = y1, x1
      x2, y2 = y2, x2
    end
    if can_reach(proxy, x1, y1, x2, y2) or iter > 20 then
      found = true
      add_base_and_workers(proxy, x1, y1, 0)
      add_base_and_workers(proxy, x2, y2, 2)
    end
  end
  local players = {0, 2}
  for p, player_id in ipairs(players) do
    for i = 1, n_resource do
      local found = false
      while not found do
        local xx = math.random(2, X - 3)
        local yy = 0
        if player_id == 0 then
          yy = math.random(2, N)
        else
          yy = Y - 1 - math.random(2, N)
        end
        -- swap the coordinates
        if dir then
          xx, yy = yy, xx
        end
        if can_pass(proxy, xx, yy) then
          proxy:send_cmd_create(UnitType.RESOURCE, __make_p(xx, yy), player_id, 0)
          found = true
        end
      end
    end
  end
end

function generate_resource(proxy, player_id)
  local X = proxy:get_x_size()
  local Y = proxy:get_y_size()
  local n_resource = 3
  for i = 1, n_resource do
    local found = false
    while not found do
      local xx = math.random(0, X - 1)
      local yy = math.random(0, Y - 1)
      if can_pass(proxy, xx, yy) then
        proxy:send_cmd_create(UnitType.RESOURCE, __make_p(xx, yy), player_id, 0)
        found = true
      end
    end
  end
end

function rts_unit_generator.generate_random(proxy, num_players, seed)
  math.randomseed(seed)
  generate_bases(proxy)

  -- change resources
  proxy:send_cmd_change_player_resource(0, 200)
  proxy:send_cmd_change_player_resource(2, 200)
end
