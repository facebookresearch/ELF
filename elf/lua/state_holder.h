#pragma once

#include "selene.h"
#include "common.h"


class StateHolder : public detail::NonMovable, detail::NonCopyable {
public:
    static sel::State& GetState() {
        // This initialization should be thread safe according to the standard.
        static sel::State state{true};
        return state;
    }

protected:
    StateHolder() = default;
    ~StateHolder() = default;
};
