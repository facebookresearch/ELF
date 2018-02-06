rts_unit_generator = {}

function __make_p(x, y)
  local p = PointF.new()
  p:set_int_xy(x, y)
  return p
end


function rts_unit_generator.generate(map, cmd_receiver, num_players, seed)
  math.randomseed(seed)
  --local X = map:get_x_size()
  --local Y = map:get_y_size()
  local player_id = 0
  local enemy_id = 1

  cmd_receiver:send_cmd_create(UnitType.RESOURCE, __make_p(2, 1), player_id, 0)
  cmd_receiver:send_cmd_create(UnitType.WORKER, __make_p(4, 4), player_id, 0)
  cmd_receiver:send_cmd_create(UnitType.WORKER, __make_p(5, 5), player_id, 0)
  cmd_receiver:send_cmd_create(UnitType.WORKER, __make_p(6, 7), player_id, 0)
  cmd_receiver:send_cmd_create(UnitType.BASE, __make_p(7, 2), player_id, 0)
  cmd_receiver:send_cmd_change_player_resource(player_id, 100)


  cmd_receiver:send_cmd_create(UnitType.RESOURCE, __make_p(14, 18), enemy_id, 0)
  cmd_receiver:send_cmd_create(UnitType.BASE, __make_p(18, 16), enemy_id, 0)
  cmd_receiver:send_cmd_change_player_resource(enemy_id, 100)
end
