#pragma once

#include <selene.h>
#include <string>

#include <sstream>


class CapturedStdout {
public:
    CapturedStdout() {
        _old = std::cout.rdbuf(_out.rdbuf());
    }

    ~CapturedStdout() {
        std::cout.rdbuf(_old);
    }

    std::string Content() const {
        return _out.str();
    }

private:
    std::stringstream _out;
    std::streambuf *_old;
};

bool test_load_error(sel::State &state) {
    CapturedStdout capture;
    const char* expected = "cannot open";
    return !state.Load("../test/non_exist.lua")
        && capture.Content().find(expected) != std::string::npos;
}

bool test_load_syntax_error(sel::State &state) {
    const char* expected = "unexpected symbol";
    CapturedStdout capture;
    return !state.Load("../test/test_syntax_error.lua")
        && capture.Content().find(expected) != std::string::npos;
}

bool test_do_syntax_error(sel::State &state) {
    const char* expected = "unexpected symbol";
    CapturedStdout capture;
    return !state("function syntax_error() 1 2 3 4 end")
        && capture.Content().find(expected) != std::string::npos;
}

bool test_call_undefined_function(sel::State &state) {
    state.Load("../test/test_error.lua");
    const char* expected = "attempt to call a nil value";
    CapturedStdout capture;
    state["undefined_function"]();
    return capture.Content().find(expected) != std::string::npos;
}

bool test_call_undefined_function2(sel::State &state) {
    state.Load("../test/test_error.lua");
#if LUA_VERSION_NUM < 503
    const char* expected = "attempt to call global 'err_func2'";
#else
    const char* expected = "attempt to call a nil value (global 'err_func2')";
#endif
    CapturedStdout capture;
    state["err_func1"](1, 2);
    return capture.Content().find(expected) != std::string::npos;
}

bool test_call_stackoverflow(sel::State &state) {
    state.Load("../test/test_error.lua");
    const char* expected = "test_error.lua:10: stack overflow";
    CapturedStdout capture;
    state["do_overflow"]();
    return capture.Content().find(expected) != std::string::npos;
}

bool test_parameter_conversion_error(sel::State &state) {
    const char * expected =
        "bad argument #2 to 'accept_string_int_string' (number expected, got string)";
    std::string largeStringToPreventSSO(50, 'x');
    state["accept_string_int_string"] = [](std::string, int, std::string){};

    CapturedStdout capture;
    state["accept_string_int_string"](
        largeStringToPreventSSO,
        "not a number",
        largeStringToPreventSSO);
    return capture.Content().find(expected) != std::string::npos;
}
