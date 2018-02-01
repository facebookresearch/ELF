#pragma once

#include "selene.h"


class StateHolder {
public:
    static sel::State& GetState() {
        // This initialization should be thread safe according to the standard.
        static sel::State state{true};
        return state;
    }

    // Disable copy/move semantics
    StateHolder(const StateHolder&) = delete;
    StateHolder& operator=(const StateHolder&) = delete;
    StateHolder(StateHolder&&) = delete;
    StateHolder&& operator=(StateHolder&&) = delete;

protected:
    StateHolder() = default;
    ~StateHolder() = default;
};
