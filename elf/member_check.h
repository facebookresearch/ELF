#pragma once

#include <type_traits>

#define MEMBER_CHECK(member) \
                        \
template <typename T, typename = int> \
struct has_##member : std::false_type { };\
\
template <typename T>\
struct has_##member <T, decltype((void) T::member, 0)> : std::true_type { };


#define MEMBER_FUNC_CHECK(func) \
\
template<typename T>\
struct has_func_##func {\
    template<typename U, size_t (U::*)() const> struct SFINAE {}; \
    template<typename U> static char Test(SFINAE<U, &U::func>*); \
    template<typename U> static int Test(...); \
    enum { value = sizeof(Test<T>(0)) == sizeof(char) }; \
};
