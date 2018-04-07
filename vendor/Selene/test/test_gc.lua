objs = {}
function make_ten()
   for i = 1, 10 do
      objs[i] = GCTest.new()
   end
end

function destroy_ten()
   for i = 1, 10 do
      objs[i] = nil
   end
end
