print(env.unit(0):info())
print(env.unit(1):info())
print(env.unit(2):info())
print(env.unit(3):info())

bar = Bar.new(5, 2)
-- bar now refers to a new instance of Bar with its member x set to 5
--
print(global_float())

x = bar:add_this(2)
print(x)
print(type(x))
-- print(bar.x)
-- x is now 7
--
print(type(bar:abs()))

y = bar:sub_this(2.0)
print(y)

loc = bar:loc()
print(loc:info())

bar = nil
--[[ the bar object will be destroyed the next time a garbage
     collection is run ]]--
