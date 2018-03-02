#pragma once

#include <selene.h>

struct Qux {
    int baz() { return 4; }
    int qux = 3;
};

static Qux qux;

Qux *GetQuxPtr() { return &qux; }
Qux &GetQuxRef() { return qux; }

bool test_metatable_registry_ptr(sel::State &state) {
    state["get_instance"] = &GetQuxPtr;
    state["Qux"].SetClass<Qux>("baz", &Qux::baz);
    state.Load("../test/test_metatable.lua");
    return state["call_method"]() == 4;
}

bool test_metatable_registry_ref(sel::State &state) {
    state["get_instance"] = &GetQuxRef;
    state["Qux"].SetClass<Qux>("baz", &Qux::baz);
    state.Load("../test/test_metatable.lua");
    return state["call_method"]() == 4;
}

bool test_metatable_ptr_member(sel::State &state) {
    state["get_instance"] = &GetQuxPtr;
    state["Qux"].SetClass<Qux>("baz", &Qux::baz, "qux", &Qux::qux);
    state.Load("../test/test_metatable.lua");
    return state["access_member"]() == 3;
}

bool test_metatable_ref_member(sel::State &state) {
    state["get_instance"] = &GetQuxRef;
    state["Qux"].SetClass<Qux>("baz", &Qux::baz, "qux", &Qux::qux);
    state.Load("../test/test_metatable.lua");
    return state["access_member"]() == 3;
}
