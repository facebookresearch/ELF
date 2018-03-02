#pragma once

#if defined(_MSC_VER) && _MSC_VER < 1900 // before MSVS-14 CTP1
#define constexpr const
#endif

#include "Selene/include/selene/State.h"
#include "Selene/include/selene/Tuple.h"
