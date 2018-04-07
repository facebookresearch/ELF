#pragma once

#include "common/lifetime.h"
#include <iostream>
#include <selene.h>

int take_fun_arg(sel::function<int(int, int)> fun, int a, int b) {
    return fun(a, b);
}

struct Mutator {
    Mutator() {}
    Mutator(sel::function<void(int)> fun) {
        fun(-4);
    }
    sel::function<void()> Foobar(bool which,
                                 sel::function<void()> foo,
                                 sel::function<void()> bar) {
        return which ? foo : bar;
    }
};

bool test_function_reference(sel::State &state) {
    state["take_fun_arg"] = &take_fun_arg;
    state.Load("../test/test_ref.lua");
    bool check1 = state["pass_add"](3, 5) == 8;
    bool check2 = state["pass_sub"](4, 2) == 2;
    return check1 && check2;
}

bool test_function_in_constructor(sel::State &state) {
    state["Mutator"].SetClass<Mutator, sel::function<void(int)>>();
    state.Load("../test/test_ref.lua");
    bool check1 = state["a"] == 4;
    state("mutator = Mutator.new(mutate_a)");
    bool check2 = state["a"] == -4;
    return check1 && check2;
}

bool test_pass_function_to_lua(sel::State &state) {
    state["Mutator"].SetClass<Mutator>("foobar", &Mutator::Foobar);
    state.Load("../test/test_ref.lua");
    state("mutator = Mutator.new()");
    state("mutator:foobar(true, foo, bar)()");
    bool check1 = state["test"] == "foo";
    state("mutator:foobar(false, foo, bar)()");
    bool check2 = state["test"] == "bar";
    return check1 && check2;
}

bool test_call_returned_lua_function(sel::State &state) {
    state.Load("../test/test_ref.lua");
    sel::function<int(int, int)> lua_add = state["add"];
    return lua_add(2, 4) == 6;
}

bool test_call_multivalue_lua_function(sel::State &state) {
    state.Load("../test/test_ref.lua");
    sel::function<std::tuple<int, int>()> lua_add = state["return_two"];
    return lua_add() == std::make_tuple(1, 2);
}

bool test_call_result_is_alive_ptr(sel::State &state) {
    using namespace test_lifetime;
    state["Obj"].SetClass<InstanceCounter>();
    state("function createObj() return Obj.new() end");
    sel::function<sel::Pointer<InstanceCounter>()> createObj = state["createObj"];
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    sel::Pointer<InstanceCounter> pointer = createObj();
    state.ForceGC();

    return InstanceCounter::instances == instanceCountBeforeCreation + 1;
}

bool test_call_result_is_alive_ref(sel::State &state) {
    using namespace test_lifetime;
    state["Obj"].SetClass<InstanceCounter>();
    state("function createObj() return Obj.new() end");
    sel::function<sel::Reference<InstanceCounter>()> createObj = state["createObj"];
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    sel::Reference<InstanceCounter> ref = createObj();
    state.ForceGC();

    return InstanceCounter::instances == instanceCountBeforeCreation + 1;
}

struct FunctionFoo {
    int x;
    FunctionFoo(int num) : x(num) {}
    int getX() {
        return x;
    }
};
struct FunctionBar {};

bool test_function_call_with_registered_class(sel::State &state) {
    state["Foo"].SetClass<FunctionFoo, int>("get", &FunctionFoo::getX);
    state("function getX(foo) return foo:get() end");
    sel::function<int(FunctionFoo &)> getX = state["getX"];
    FunctionFoo foo{4};
    return getX(foo) == 4;
}

bool test_function_call_with_registered_class_ptr(sel::State &state) {
    state["Foo"].SetClass<FunctionFoo, int>("get", &FunctionFoo::getX);
    state("function getX(foo) return foo:get() end");
    sel::function<int(FunctionFoo *)> getX = state["getX"];
    FunctionFoo foo{4};
    return getX(&foo) == 4;
}

bool test_function_call_with_registered_class_val(sel::State &state) {
    state["Foo"].SetClass<FunctionFoo, int>("get", &FunctionFoo::getX);
    state("function store(foo) globalFoo = foo end");
    state("function getX() return globalFoo:get() end");

    sel::function<void(FunctionFoo)> store = state["store"];
    sel::function<int()> getX = state["getX"];
    store(FunctionFoo{4});

    return getX() == 4;
}

bool test_function_call_with_registered_class_val_lifetime(sel::State &state) {
    using namespace test_lifetime;
    state["Foo"].SetClass<InstanceCounter>();
    state("function store(foo) globalFoo = foo end");
    sel::function<void(InstanceCounter)> store = state["store"];

    int instanceCountBefore = InstanceCounter::instances;
    store(InstanceCounter{});

    return InstanceCounter::instances == instanceCountBefore + 1;
}

bool test_function_call_with_nullptr_ref(sel::State &state) {
    state["Foo"].SetClass<FunctionFoo, int>();
    state("function makeNil() return nil end");
    sel::function<FunctionFoo &()> getFoo = state["makeNil"];
    bool error_encounted = false;

    try {
        FunctionFoo & foo = getFoo();
    } catch(sel::TypeError &) {
        error_encounted = true;
    }

    return error_encounted;
}

bool test_function_call_with_wrong_ref(sel::State &state) {
    state["Foo"].SetClass<FunctionFoo, int>();
    state["Bar"].SetClass<FunctionBar>();
    state("function makeBar() return Bar.new() end");
    sel::function<FunctionFoo &()> getFoo = state["makeBar"];
    bool error_encounted = false;

    try {
        FunctionFoo & foo = getFoo();
    } catch(sel::TypeError &) {
        error_encounted = true;
    }

    return error_encounted;
}

bool test_function_call_with_wrong_ptr(sel::State &state) {
    state["Foo"].SetClass<FunctionFoo, int>();
    state["Bar"].SetClass<FunctionBar>();
    state("function makeBar() return Bar.new() end");
    sel::function<FunctionFoo *()> getFoo = state["makeBar"];
    return nullptr == getFoo();
}

bool test_function_get_registered_class_by_value(sel::State &state) {
    state["Foo"].SetClass<FunctionFoo, int>();
    state("function getFoo() return Foo.new(4) end");
    sel::function<FunctionFoo()> getFoo = state["getFoo"];

    FunctionFoo foo = getFoo();

    return foo.getX() == 4;
}
