function add(a, b)
   return a+b
end

function subtract(a, b)
   return a-b
end

function pass_add(x, y)
   return take_fun_arg(add, x, y)
end

function pass_sub(x, y)
   return take_fun_arg(subtract, x, y)
end

a = 4

function mutate_a(new_a)
   a = new_a
end

test = ""

function foo()
   test = "foo"
end

function bar()
   test = "bar"
end

function return_two()
   return 1, 2
end
