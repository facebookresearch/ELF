#pragma once

#include <exception>
#include <lua.h>
#include <selene.h>

bool test_catch_exception_from_callback_within_lua(sel::State &state) {
    state.Load("../test/test_exceptions.lua");
    state["throw_logic_error"] =
        []() {throw std::logic_error("Message from C++.");};
    bool ok = true;
    std::string msg;
    sel::tie(ok, msg) = state["call_protected"]("throw_logic_error");
    return !ok && msg.find("Message from C++.") != std::string::npos;
}

bool test_catch_unknwon_exception_from_callback_within_lua(sel::State &state) {
    state.Load("../test/test_exceptions.lua");
    state["throw_int"] =
        []() {throw 0;};
    bool ok = true;
    std::string msg;
    sel::tie(ok, msg) = state["call_protected"]("throw_int");
    return !ok && msg.find("<Unknown exception>") != std::string::npos;
}

bool test_call_exception_handler_for_exception_from_lua(sel::State &state) {
    state.Load("../test/test_exceptions.lua");
    int luaStatusCode = LUA_OK;
    std::string message;
    state.HandleExceptionsWith([&luaStatusCode, &message](int s, std::string msg, std::exception_ptr exception) {
        luaStatusCode = s, message = std::move(msg);
    });
    state["raise"]("Message from Lua.");
    return luaStatusCode == LUA_ERRRUN
        && message.find("Message from Lua.") != std::string::npos;
}

bool test_call_exception_handler_for_exception_from_callback(sel::State &state) {
    int luaStatusCode = LUA_OK;
    std::string message;
    state.HandleExceptionsWith([&luaStatusCode, &message](int s, std::string msg, std::exception_ptr exception) {
        luaStatusCode = s, message = std::move(msg);
    });
    state["throw_logic_error"] =
        []() {throw std::logic_error("Message from C++.");};
    state["throw_logic_error"]();
    return luaStatusCode == LUA_ERRRUN
        && message.find("Message from C++.") != std::string::npos;
}

bool test_call_exception_handler_while_using_sel_function(sel::State &state) {
    state.Load("../test/test_exceptions.lua");
    int luaStatusCode = LUA_OK;
    std::string message;
    state.HandleExceptionsWith([&luaStatusCode, &message](int s, std::string msg, std::exception_ptr exception) {
        luaStatusCode = s, message = std::move(msg);
    });
    sel::function<void(std::string)> raiseFromLua = state["raise"];
    raiseFromLua("Message from Lua.");
    return luaStatusCode == LUA_ERRRUN
        && message.find("Message from Lua.") != std::string::npos;
}

bool test_rethrow_exception_for_exception_from_callback(sel::State &state) {
    state.HandleExceptionsWith([](int s, std::string msg, std::exception_ptr exception) {
        if(exception) {
            std::rethrow_exception(exception);
        }
    });
    state["throw_logic_error"] =
        []() {throw std::logic_error("Arbitrary message.");};
    try {
        state["throw_logic_error"]();
    } catch(std::logic_error & e) {
        return std::string(e.what()).find("Arbitrary message.") != std::string::npos;
    }
    return false;
}

bool test_rethrow_using_sel_function(sel::State & state) {
    state.HandleExceptionsWith([](int s, std::string msg, std::exception_ptr exception) {
        if(exception) {
            std::rethrow_exception(exception);
        }
    });
    state["throw_logic_error"] =
        []() {throw std::logic_error("Arbitrary message.");};
    sel::function<void(void)> cause_exception = state["throw_logic_error"];
    try {
        cause_exception();
    } catch(std::logic_error & e) {
        return std::string(e.what()).find("Arbitrary message.") != std::string::npos;
    }
    return false;
}

bool test_throw_on_exception_using_Load(sel::State &state) {
    state.HandleExceptionsWith([](int s, std::string msg, std::exception_ptr exception) {
        throw std::logic_error(msg);
    });
    try {
        state.Load("non_existing_file");
    } catch (std::logic_error & e) {
        return std::string(e.what()).find("non_existing_file") != std::string::npos;
    }
    return false;
}
