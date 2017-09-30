#pragma once

#include <type_traits>

#define MEMBER_CHECK(member) \
                        \
template <typename __T, typename = int> \
struct has_##member : std::false_type { };\
\
template <typename __T>\
struct has_##member <__T, decltype((void) __T::member, 0)> : std::true_type { };


#define MEMBER_FUNC_CHECK(func) \
\
template<typename __T>\
struct has_func_##func {\
    template<typename __U, size_t (__U::*)() const> struct SFINAE {}; \
    template<typename __U> static char Test(SFINAE<__U, &__U::func>*); \
    template<typename __U> static int Test(...); \
    enum { value = sizeof(Test<__T>(0)) == sizeof(char) }; \
};
