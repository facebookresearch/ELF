#pragma once

#include "common/lifetime.h"
#include <selene.h>

bool test_select_global(sel::State &state) {
    state.Load("../test/test.lua");
    int answer = state["my_global"];
    return answer == 4;
}

bool test_select_field(sel::State &state) {
    state.Load("../test/test.lua");
    lua_Number answer = state["my_table"]["key"];
    return answer == lua_Number(6.4);
}

bool test_select_index(sel::State &state) {
    state.Load("../test/test.lua");
    std::string answer = state["my_table"][3];
    return answer == "hi";
}

bool test_select_nested_field(sel::State &state) {
    state.Load("../test/test.lua");
    std::string answer = state["my_table"]["nested"]["foo"];
    return answer == "bar";
}

bool test_select_nested_index(sel::State &state) {
    state.Load("../test/test.lua");
    int answer = state["my_table"]["nested"][2];
    return answer == -3;
}

bool test_select_equality(sel::State &state) {
    state.Load("../test/test.lua");
    return state["my_table"]["nested"][2] == -3;
}

bool test_select_cast(sel::State &state) {
    state.Load("../test/test.lua");
    return int(state["global1"]) == state["global2"];
}

bool test_set_global(sel::State &state) {
    state.Load("../test/test.lua");
    auto lua_dummy_global = state["dummy_global"];
    lua_dummy_global = 32;
    return state["dummy_global"] == 32;
}

bool test_set_field(sel::State &state) {
    state.Load("../test/test.lua");
    state["my_table"]["dummy_key"] = "testing";
    return state["my_table"]["dummy_key"] == "testing";
}

bool test_set_index(sel::State &state) {
    state.Load("../test/test.lua");
    state["my_table"][10] = 3;
    return state["my_table"][10] == 3;
}

bool test_set_nested_field(sel::State &state) {
    state.Load("../test/test.lua");
    state["my_table"]["nested"]["asdf"] = true;
    return state["my_table"]["nested"]["asdf"];
}

bool test_set_nested_index(sel::State &state) {
    state.Load("../test/test.lua");
    state["my_table"]["nested"][1] = 2;
    return state["my_table"]["nested"][1] == 2;
}

bool test_create_table_field(sel::State &state) {
    state["new_table"]["test"] = 4;
    return state["new_table"]["test"] == 4;
}

bool test_create_table_index(sel::State &state) {
    state["new_table"][3] = 4;
    return state["new_table"][3] == 4;
}

bool test_cache_selector_field_assignment(sel::State &state) {
    sel::Selector s = state["new_table"][3];
    s = 4;
    return state["new_table"][3] == 4;
}

bool test_cache_selector_field_access(sel::State &state) {
    state["new_table"][3] = 4;
    sel::Selector s = state["new_table"][3];
    return s == 4;
}

bool test_cache_selector_function(sel::State &state) {
    state.Load("../test/test.lua");
    sel::Selector s = state["set_global"];
    s();
    return state["global1"] == 8;
}

bool test_function_should_run_once(sel::State &state) {
    state.Load("../test/test.lua");
    auto should_run_once = state["should_run_once"];
    should_run_once();
    return state["should_be_one"] == 1;
}

bool test_function_result_is_alive_ptr(sel::State &state) {
    using namespace test_lifetime;
    state["Obj"].SetClass<InstanceCounter>();
    state("function createObj() return Obj.new() end");
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    sel::Pointer<InstanceCounter> pointer = state["createObj"]();
    state.ForceGC();

    return InstanceCounter::instances == instanceCountBeforeCreation + 1;
}

bool test_function_result_is_alive_ref(sel::State &state) {
    using namespace test_lifetime;
    state["Obj"].SetClass<InstanceCounter>();
    state("function createObj() return Obj.new() end");
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    sel::Reference<InstanceCounter> reference = state["createObj"]();
    state.ForceGC();

    return InstanceCounter::instances == instanceCountBeforeCreation + 1;
}

bool test_get_and_set_Reference_keeps_identity(sel::State &state) {
    using namespace test_lifetime;
    state["Obj"].SetClass<InstanceCounter>();
    state("objA = Obj.new()");

    sel::Reference<InstanceCounter> objA_ref = state["objA"];
    state["objB"] = objA_ref;
    sel::Reference<InstanceCounter> objB_ref = state["objB"];

    state("function areVerySame() return objA == objB end");
    return state["areVerySame"]() && (&objA_ref.get() == &objB_ref.get());
}

bool test_get_and_set_Pointer_keeps_identity(sel::State &state) {
    using namespace test_lifetime;
    state["Obj"].SetClass<InstanceCounter>();
    state("objA = Obj.new()");

    sel::Pointer<InstanceCounter> objA_ptr = state["objA"];
    state["objB"] = objA_ptr;
    sel::Pointer<InstanceCounter> objB_ptr = state["objB"];

    state("function areVerySame() return objA == objB end");
    return state["areVerySame"]() && (objA_ptr == objB_ptr);
}

struct SelectorBar {};
struct SelectorFoo {
    int x;
    SelectorFoo(int num) : x(num) {}
    int getX() {
        return x;
    }
};

bool test_selector_call_with_registered_class(sel::State &state) {
    state["Foo"].SetClass<SelectorFoo, int>("get", &SelectorFoo::getX);
    state("function getXFromFoo(foo) return foo:get() end");
    SelectorFoo foo{4};
    return state["getXFromFoo"](foo) == 4;
}

bool test_selector_call_with_registered_class_ptr(sel::State &state) {
    state["Foo"].SetClass<SelectorFoo, int>("get", &SelectorFoo::getX);
    state("function getXFromFoo(foo) return foo:get() end");
    SelectorFoo foo{4};
    return state["getXFromFoo"](&foo) == 4;
}

bool test_selector_call_with_wrong_type_ptr(sel::State &state) {
    auto acceptFoo = [] (SelectorFoo *) {};
    state["Foo"].SetClass<SelectorFoo, int>();
    state["Bar"].SetClass<SelectorBar>();
    state["acceptFoo"] = acceptFoo;
    state("bar = Bar.new()");

    bool error_encounted = false;
    state.HandleExceptionsWith([&error_encounted](int, std::string, std::exception_ptr) {
        error_encounted = true;
    });
    state("acceptFoo(bar)");

    return error_encounted;
}

bool test_selector_call_with_wrong_type_ref(sel::State &state) {
    auto acceptFoo = [] (SelectorFoo &) {};
    state["Foo"].SetClass<SelectorFoo, int>();
    state["Bar"].SetClass<SelectorBar>();
    state["acceptFoo"] = acceptFoo;
    state("bar = Bar.new()");

    bool error_encounted = false;
    state.HandleExceptionsWith([&error_encounted](int, std::string, std::exception_ptr) {
        error_encounted = true;
    });
    state("acceptFoo(bar)");

    return error_encounted;
}

bool test_selector_call_with_nullptr_ref(sel::State &state) {
    auto acceptFoo = [] (SelectorFoo &) {};
    state["Foo"].SetClass<SelectorFoo, int>();
    state["acceptFoo"] = acceptFoo;

    bool error_encounted = false;
    state.HandleExceptionsWith([&error_encounted](int, std::string, std::exception_ptr) {
        error_encounted = true;
    });
    state("acceptFoo(nil)");

    return error_encounted;
}

bool test_selector_get_nullptr_ref(sel::State &state) {
    state["Foo"].SetClass<SelectorFoo, int>();
    state("bar = nil");
    bool error_encounted = false;

    try{
        SelectorFoo & foo = state["bar"];
    } catch(sel::TypeError &) {
        error_encounted = true;
    }

    return error_encounted;
}

bool test_selector_get_wrong_ref(sel::State &state) {
    state["Foo"].SetClass<SelectorFoo, int>();
    state["Bar"].SetClass<SelectorBar>();
    state("bar = Bar.new()");
    bool error_encounted = false;

    try{
        SelectorFoo & foo = state["bar"];
    } catch(sel::TypeError &) {
        error_encounted = true;
    }

    return error_encounted;
}

bool test_selector_get_wrong_ref_to_string(sel::State &state) {
    state["Foo"].SetClass<SelectorFoo, int>();
    state("bar = \"Not a Foo\"");
    bool expected_message = false;

    try{
        SelectorFoo & foo = state["bar"];
    } catch(sel::TypeError & e) {
        expected_message = std::string(e.what()).find("got string") != std::string::npos;
    }

    return expected_message;
}

bool test_selector_get_wrong_ref_to_table(sel::State &state) {
    state["Foo"].SetClass<SelectorFoo, int>();
    state("bar = {}");
    bool expected_message = false;

    try{
        SelectorFoo & foo = state["bar"];
    } catch(sel::TypeError & e) {
        expected_message = std::string(e.what()).find("got table") != std::string::npos;
    }

    return expected_message;
}

bool test_selector_get_wrong_ref_to_unregistered(sel::State &state) {
    state["Foo"].SetClass<SelectorFoo, int>();
    state("foo = Foo.new(4)");
    bool expected_message = false;

    try{
        SelectorBar & bar = state["foo"];
    } catch(sel::TypeError & e) {
        expected_message = std::string(e.what()).find("unregistered type expected") != std::string::npos;
    }

    return expected_message;
}

bool test_selector_get_wrong_ptr(sel::State &state) {
    state["Foo"].SetClass<SelectorFoo, int>();
    state["Bar"].SetClass<SelectorBar>();
    state("bar = Bar.new()");
    SelectorFoo * foo = state["bar"];
    return foo == nullptr;
}
