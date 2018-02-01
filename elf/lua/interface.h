#pragma once

#include <unordered_map>
#include <type_traits>

#include "state_holder.h"


// Use this class to expose a C++ type to Lua
template <typename T>
struct LuaInterface {
    using ClassT = T;

    LuaInterface(const LuaInterface&) = delete;
    LuaInterface& operator=(const LuaInterface&) = delete;
    LuaInterface(LuaInterface&&) = delete;
    LuaInterface&& operator=(LuaInterface&&) = delete;

    template <typename... Args>
    static void Register(const char* name, Args&&... args) {
        auto& state = StateHolder::GetState();
        state[name].SetClass<ClassT>(std::forward<Args>(args)...);
    }
};

template <typename T>
class CppInterface {
public:
    using ClassT = T;

protected:
    template <typename R, typename... Args>
    static typename std::enable_if<std::is_same<R, void>::value, void>::type
    Invoke(const char* class_name, const char* name, Args&&... args) {
        auto& state = StateHolder::GetState();
        state[class_name][name](std::forward<Args>(args)...);
    }

    template <typename R, typename... Args>
    void Invoke(const char* class_name, const char* name, R* const ret, Args&&... args) {
        auto& state = StateHolder::GetState();
        *ret = state[class_name][name](std::forward<Args>(args)...);
    }

    static void init(const char* file_name) {
        auto& state = StateHolder::GetState();
        state.Load(file_name);
    }
};
