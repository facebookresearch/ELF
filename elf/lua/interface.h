#pragma once

#include <unordered_map>
#include <type_traits>

#include "state_holder.h"
#include "common.h"


// Use this class to expose a C++ type to Lua
template <typename T>
struct LuaClassInterface : public detail::NonCopyable, detail::NonMovable {
    using ClassT = T;

    template <typename... Args>
    static void Register(const char* name, Args&&... args) {
        auto& state = StateHolder::GetState();
        state[name].SetClass<ClassT>(std::forward<Args>(args)...);
    }
};


template <typename T>
struct LuaEnumInterface : public detail::NonCopyable, detail::NonMovable {
    using EnumT = T;

    template <typename Idx2StrFuncT>
    static void Register(const char* name, Idx2StrFuncT idx2str, int N, int start_from = 0) {
        auto& state = StateHolder::GetState();
        for (int i = start_from; i <= N; ++i) {
            const auto& field = idx2str(static_cast<EnumT>(i));
            state[name][field.c_str()] = i;
        }
    }

    static void Register(const char* name, const char* field, int value) {
        auto& state = StateHolder::GetState();
        state[name][field] = value;
    }
};


template <typename T>
class CppClassInterface : public detail::NonCopyable, detail::NonMovable {
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
    static void Invoke(const char* class_name, const char* name, R* const ret, Args&&... args) {
        auto& state = StateHolder::GetState();
        *ret = state[class_name][name](std::forward<Args>(args)...);
    }

    static void init(const char* file_name) {
        auto& state = StateHolder::GetState();
        state.Load(file_name);
    }
};
