

local rts_map_parser = {}
rts_map_parser.__index = rts_map_parser

function rts_map_parser:new(file_name)
  local self = {}
  setmetatable(self, rts_map_parser)
  self.map = {}
  self.X = 0
  self.Y = 0
  for line in io.lines(file_name) do
    if line:sub(1, 1) ~= '#' then
      table.insert(self.map, line)
      self.Y = #line
      self.X = self.X + 1
    end
  end
  return self
end

function rts_map_parser:get_x_size()
  return self.X
end

function rts_map_parser:get_y_size()
  return self.Y
end

function rts_map_parser:get_locations(ty)
  local xs = {}
  local ys = {}
  for y = 1, self.Y do
    for x = 1, self.X do
      if self.map[y]:sub(x, x) == ty then
        table.insert(xs, x)
        table.insert(ys, y)
      end
    end
  end
  return xs, ys
end

function rts_map_parser:get_soil_locations()
  return self:get_locations('.')
end

function rts_map_parser:get_sand_locations()
  return self:get_locations('1')
end

function rts_map_parser:get_grass_locations()
  return self:get_locations('2')
end

function rts_map_parser:get_rock_locations()
  return self:get_locations('3')
end

function rts_map_parser:get_water_locations()
  return self:get_locations('4')
end


return rts_map_parser
