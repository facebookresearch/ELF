#pragma once

namespace detail {

struct NonCopyable {
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

struct NonMovable {
    NonMovable(NonMovable&&) = delete;
    NonMovable&& operator=(NonMovable&&) = delete;
};

}
