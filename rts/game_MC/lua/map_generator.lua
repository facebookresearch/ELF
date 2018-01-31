
-- integrate it into lua state
terrain = {
  INVALID_TERRAIN = -1,
  NORMAL = 0,
  IMPASSABLE = 1,
  FOG = 2
}

rts_map = {}

function rts_map.generate_impassable(map, k)
  for i = 1, k do
    local x = math.random(0, map:m() - 1)
    local y = math.random(0, map:n() - 1)
    map:set_slot_type(terrain.IMPASSABLE, x, y, 0)
  end
end

function rts_map.generate_base(map)
  local x = math.random(0, map:m() - 1)
  local y = math.random(0, map:n() - 1)
  return x, y
end


function g_generate(map, num_players, init_resource)
    map:clear_map()
    map:init_map(20, 20, 1)
    rts_map.generate_impassable(map, 0)
    for player_id = 0, num_players - 1 do
      local base_x, base_y = rts_map.generate_base(map)
      local resource_x, resource_y = rts_map.generate_base(map)
      map:add_player(player_id, base_x, base_y, resource_x, resource_y, init_resource)
    end
    map:reset_intermediates()
end
