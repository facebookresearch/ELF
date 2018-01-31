#pragma once

#include "selene.h"

template <typename DerivedT>
struct LuaInterface {
  static void ExposeInterface(const std::string& type_name, sel::State& state) {
    return DerivedT::ExposeInterfaceImpl(type_name, state);
  }
};

class LuaContext {
public:
  template <typename T>
  static void RegisterCppType(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    T::ExposeInterface(name, state_);
  }

  static void RegisterLuaFunc(const std::string& file_name);

  template <typename... Args>
  static void Run(const std::string& func_name, Args&&... args) {
    state_[func_name.c_str()](std::forward<Args>(args)...);
  }

private:

  static sel::State state_;

  static std::mutex mutex_;
};


// Call this once to create lua context
void reg_lua_context();
