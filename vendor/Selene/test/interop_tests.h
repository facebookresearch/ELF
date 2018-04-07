#pragma once

#include "common/lifetime.h"
#include <memory>
#include <selene.h>
#include <string>

int my_add(int a, int b) {
    return a + b;
}

void no_return() {
}

bool test_function_no_args(sel::State &state) {
    state.Load("../test/test.lua");
    state["foo"]();
    return true;
}

bool test_add(sel::State &state) {
    state.Load("../test/test.lua");
    return state["add"](5, 2) == 7;
}

bool test_multi_return(sel::State &state) {
    state.Load("../test/test.lua");
    int sum, difference;
    sel::tie(sum, difference) = state["sum_and_difference"](3, 1);
    return (sum == 4 && difference == 2);
}

bool test_multi_return_invoked_once(sel::State &state) {
    int invocation_count = 0;
    state["two_ints"] = [&invocation_count](){
        ++invocation_count;
        return std::tuple<int, int>{1, 2};
    };
    int res_a = 0, res_b = 0;
    sel::tie(res_a, res_b) = state["two_ints"]();
    return invocation_count == 1;
}

bool test_heterogeneous_return(sel::State &state) {
    state.Load("../test/test.lua");
    int x;
    bool y;
    std::string z;
    sel::tie(x, y, z) = state["bar"]();
    return x == 4 && y == true && z == "hi";
}

bool test_call_field(sel::State &state) {
    state.Load("../test/test.lua");
    int answer = state["mytable"]["foo"]();
    return answer == 4;
}

bool test_call_c_function(sel::State &state) {
    state.Load("../test/test.lua");
    state["cadd"] = std::function<int(int, int)>(my_add);
    int answer = state["cadd"](3, 6);
    return answer == 9;
}

bool test_call_c_fun_from_lua(sel::State &state) {
    state.Load("../test/test.lua");
    state["cadd"] = std::function<int(int, int)>(my_add);
    int answer = state["execute"]();
    return answer == 11;
}

bool test_no_return(sel::State &state) {
    state["no_return"] = &no_return;
    state["no_return"]();
    return true;
}

bool test_call_std_fun(sel::State &state) {
    state.Load("../test/test.lua");
    std::function<int(int, int)> mult = [](int x, int y){ return x * y; };
    state["cmultiply"] = mult;
    int answer = state["cmultiply"](5, 6);
    return answer == 30;
}

bool test_call_lambda(sel::State &state) {
    state.Load("../test/test.lua");
    state["cmultiply"] = [](int x, int y){ return x * y; };
    int answer = state["cmultiply"](5, 6);
    return answer == 30;
}

bool test_call_normal_c_fun(sel::State &state) {
    state.Load("../test/test.lua");
    state["cadd"] = &my_add;
    const int answer = state["cadd"](4, 20);
    return answer == 24;
}

bool test_call_normal_c_fun_many_times(sel::State &state) {
    // Ensures there isn't any strange overflow problem or lingering
    // state
    state.Load("../test/test.lua");
    state["cadd"] = &my_add;
    bool result = true;
    for (int i = 0; i < 25; ++i) {
        const int answer = state["cadd"](4, 20);
        result = result && (answer == 24);
    }
    return result;
}

bool test_call_functor(sel::State &state) {
    struct the_answer {
        int answer = 42;
        int operator()() {
            return answer;
        }
    };
    the_answer functor;
    state.Load("../test/test.lua");
    state["c_the_answer"] = std::function<int()>(functor);
    int answer = state["c_the_answer"]();
    return answer == 42;

}

std::tuple<int, int> my_sum_and_difference(int x, int y) {
    return std::make_tuple(x+y, x-y);
}

bool test_multivalue_c_fun_return(sel::State &state) {
    state.Load("../test/test.lua");
    state["test_fun"] = &my_sum_and_difference;
    int sum, difference;
    sel::tie(sum, difference) = state["test_fun"](-2, 2);
    return sum == 0 && difference == -4;
}

bool test_multivalue_c_fun_from_lua(sel::State &state) {
    state.Load("../test/test.lua");
    state["doozy_c"] = &my_sum_and_difference;
    int answer = state["doozy"](5);
    return answer == -75;
}

bool test_embedded_nulls(sel::State &state) {
    state.Load("../test/test.lua");
    const std::string result = state["embedded_nulls"]();
    return result.size() == 4;
}

bool test_coroutine(sel::State &state) {
    state.Load("../test/test.lua");
    bool check1 = state["resume_co"]() == 1;
    bool check2 = state["resume_co"]() == 2;
    bool check3 = state["resume_co"]() == 3;
    return check1 && check2 && check3;
}

struct Special {
    int foo = 3;
};

static Special special;

Special* return_special_pointer() { return &special; }

bool test_pointer_return(sel::State &state) {
    state["return_special_pointer"] = &return_special_pointer;
    return state["return_special_pointer"]() == &special;
}

Special& return_special_reference() { return special; }

bool test_reference_return(sel::State &state) {
    state["return_special_reference"] = &return_special_reference;
    Special &ref = state["return_special_reference"]();
    return &ref == &special;
}

test_lifetime::InstanceCounter return_value() { return {}; }

bool test_return_value(sel::State &state) {
    using namespace test_lifetime;
    state["MyClass"].SetClass<InstanceCounter>();
    state["return_value"] = &return_value;
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    state("globalValue = return_value()");

    return InstanceCounter::instances == instanceCountBeforeCreation + 1;
}

bool test_return_unregistered_type(sel::State &state) {
    using namespace test_lifetime;
    state["return_value"] = &return_value;
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    bool error_encounted = false;
    state.HandleExceptionsWith([&error_encounted](int, std::string msg, std::exception_ptr) {
        error_encounted = true;
    });

    state("globalValue = return_value()");

    return error_encounted;
}

bool test_value_parameter(sel::State &state) {
    using namespace test_lifetime;
    state["MyClass"].SetClass<InstanceCounter>();
    state("function acceptValue(value) valCopy = value end");
    int const instanceCountBefore = InstanceCounter::instances;

    state["acceptValue"](InstanceCounter{});

    return InstanceCounter::instances == instanceCountBefore + 1;
}

bool test_wrong_value_parameter(sel::State &state) {
    using namespace test_lifetime;
    state["MyClass"].SetClass<InstanceCounter>();
    state("function acceptValue(value) valCopy = value end");
    int const instanceCountBefore = InstanceCounter::instances;

    try {
        state["acceptValue"](Special{});
    } catch(sel::CopyUnregisteredType & e)
    {
        return e.getType().get() == typeid(Special);
    }

    return false;
}

bool test_value_parameter_keeps_type_info(sel::State &state) {
    using namespace test_lifetime;
    state["MyClass"].SetClass<Special>();
    state("function acceptValue(value) valCopy = value end");
    state["acceptValue"](Special{});

    Special * foo = state["valCopy"];

    return foo != nullptr;
}

bool test_callback_with_value(sel::State &state) {
    using namespace test_lifetime;
    state["MyClass"].SetClass<InstanceCounter>();
    state("val = MyClass.new()");

    std::unique_ptr<InstanceCounter> copy;
    state["accept"] = [&copy](InstanceCounter counter) {
        copy.reset(new InstanceCounter(std::move(counter)));
    };

    int const instanceCountBeforeCall = InstanceCounter::instances;
    state("accept(val)");

    return InstanceCounter::instances == instanceCountBeforeCall + 1;
}

bool test_nullptr_to_nil(sel::State &state) {
    state["getNullptr"] = []() -> void* {
        return nullptr;
    };
    state("x = getNullptr()");
    state("result = x == nil");
    return static_cast<bool>(state["result"]);
}

bool test_get_primitive_by_value(sel::State & state) {
    state.Load("../test/test.lua");
    return static_cast<int>(state["global1"]) == 5;
}

bool test_get_primitive_by_const_ref(sel::State & state) {
    state.Load("../test/test.lua");
    return static_cast<const int &>(state["global1"]) == 5;
}

bool test_get_primitive_by_rvalue_ref(sel::State & state) {
    state.Load("../test/test.lua");
    return static_cast<int &&>(state["global1"]) == 5;
}

bool test_call_with_primitive_by_value(sel::State & state) {
    bool success = false;
    auto const accept_int_by_value = [&success](int x) {success = x == 5;};
    state["test"] = accept_int_by_value;
    state["test"](5);
    return success;
}

bool test_call_with_primitive_by_const_ref(sel::State & state) {
    bool success = false;
    auto const accept_int_by_const_ref =
        [&success](const int & x) {success = x == 5;};
    state["test"] = accept_int_by_const_ref;
    state["test"](5);
    return success;
}

bool test_call_with_primitive_by_rvalue_ref(sel::State & state) {
    bool success = false;
    auto const accept_int_by_rvalue_ref =
        [&success](int && x) {success = x == 5;};
    state["test"] = accept_int_by_rvalue_ref;
    state["test"](5);
    return success;
}
