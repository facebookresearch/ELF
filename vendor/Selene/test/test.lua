function foo()
end

function add(a, b)
   return a + b
end

function sum_and_difference(a, b)
   return (a+b), (a-b)
end

function bar()
   return 4, true, "hi"
end

function execute()
   return cadd(5, 6);
end

function doozy(a)
   x, y = doozy_c(a, 2 * a)
   return x * y
end

mytable = {}
function mytable.foo()
   return 4
end

function embedded_nulls()
   return "\0h\0i"
end

my_global = 4

my_table = {}
my_table[3] = "hi"
my_table["key"] = 6.4

nested_table = {}
nested_table[2] = -3;
nested_table["foo"] = "bar";

my_table["nested"] = nested_table

global1 = 5
global2 = 5

function resumable()
   coroutine.yield(1)
   coroutine.yield(2)
   return 3
end

co = coroutine.create(resumable)

function resume_co()
   ran, value = coroutine.resume(co)
   return value
end

function set_global()
   global1 = 8
end

should_be_one = 0

function should_run_once()
   should_be_one = should_be_one + 1
end