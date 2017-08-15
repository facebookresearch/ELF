/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

//File: macros.h
//Author: Yuxin Wu <ppwwyyxxc@gmail.com>

#pragma once

#define MM_CONCAT__( a, b )     a ## b
#define MM_CONCAT_( a, b )      MM_CONCAT__( a, b )
#define MM_CONCAT( a, b )       MM_CONCAT_( a, b )

#define MM_INVOKE( macro, args ) macro args
#define MM_INVOKE_B( macro, args ) macro args     // For nested invocation with g++.

// apply macro with a fixed first argument (C)
#define MM_APPLY_1( macroname, C, a1 ) \
      MM_INVOKE_B( macroname, (C, a1) )

#define MM_APPLY_2( macroname, C, a1, a2 ) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_1( macroname, C, a2 )

#define MM_APPLY_3( macroname, C, a1, a2, a3 ) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_2( macroname, C, a2, a3 )

#define MM_APPLY_4( macroname, C, a1, a2, a3, a4 ) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_3( macroname, C, a2, a3, a4)

#define MM_APPLY_5( macroname, C, a1, a2, a3, a4, a5 ) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_4( macroname, C, a2, a3, a4, a5)

#define MM_APPLY_6( macroname, C, a1, a2, a3, a4, a5, a6 ) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_5( macroname, C, a2, a3, a4, a5, a6)

#define MM_APPLY_7( macroname, C, a1, a2, a3, a4, a5, a6, a7 ) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_6( macroname, C, a2, a3, a4, a5, a6, a7)

#define MM_APPLY_8( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8 ) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_7( macroname, C, a2, a3, a4, a5, a6, a7, a8)

#define MM_APPLY_9( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_8( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9)

#define MM_APPLY_10( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_9( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10)

#define MM_APPLY_11( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_10( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11)

#define MM_APPLY_12( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_11( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12)

#define MM_APPLY_13( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_12( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13)

#define MM_APPLY_14( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_13( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14)

#define MM_APPLY_15( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_14( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15)

#define MM_APPLY_16( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_15( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16)

#define MM_APPLY_17( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_16( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17)

#define MM_APPLY_18( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_17( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18)

#define MM_APPLY_19( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_18( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19)

#define MM_APPLY_20( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20) \
      MM_INVOKE_B( macroname, (C, a1) ) \
    MM_APPLY_19( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20)

#define MM_NARG(...) \
           MM_NARG_(__VA_ARGS__,MM_RSEQ_N())
#define MM_NARG_(...) \
           MM_ARG_N(__VA_ARGS__)
#define MM_ARG_N( \
              _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
             _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
             _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
             _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
             _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
             _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
             _61,_62,_63,N,...) N
#define MM_RSEQ_N() \
           63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

#define MM_APPLY( macroname, C, ... ) \
      MM_INVOKE( \
                  MM_CONCAT( MM_APPLY_, MM_NARG( __VA_ARGS__ ) ), \
                  ( macroname, C, __VA_ARGS__ ) \
                  )

#define MM_APPLY_COMMA( macroname, C, ... ) \
        MM_INVOKE( \
                    MM_CONCAT( MM_APPLY_COMMA_, MM_NARG( __VA_ARGS__ ) ), \
                    ( macroname, C, __VA_ARGS__ ) \
                    )

#define MM_APPLY_COMMA_1( macroname, C, a1 ) \
        MM_INVOKE_B( macroname, (C, a1) )

#define MM_APPLY_COMMA_2( macroname, C, a1, a2 ) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_1( macroname, C, a2 )

#define MM_APPLY_COMMA_3( macroname, C, a1, a2, a3 ) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_2( macroname, C, a2, a3 )

#define MM_APPLY_COMMA_4( macroname, C, a1, a2, a3, a4 ) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_3( macroname, C, a2, a3, a4)

#define MM_APPLY_COMMA_5( macroname, C, a1, a2, a3, a4, a5 ) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_4( macroname, C, a2, a3, a4, a5)

#define MM_APPLY_COMMA_6( macroname, C, a1, a2, a3, a4, a5, a6 ) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_5( macroname, C, a2, a3, a4, a5, a6)

#define MM_APPLY_COMMA_7( macroname, C, a1, a2, a3, a4, a5, a6, a7 ) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_6( macroname, C, a2, a3, a4, a5, a6, a7)

#define MM_APPLY_COMMA_8( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8 ) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_7( macroname, C, a2, a3, a4, a5, a6, a7, a8)

#define MM_APPLY_COMMA_9( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9 ) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_8( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9)

#define MM_APPLY_COMMA_10( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_9( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10)

#define MM_APPLY_COMMA_11( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_10( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11)

#define MM_APPLY_COMMA_12( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_11( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12)

#define MM_APPLY_COMMA_13( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_12( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13)

#define MM_APPLY_COMMA_14( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_13( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14)

#define MM_APPLY_COMMA_15( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_14( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15)

#define MM_APPLY_COMMA_16( macroname, C, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) \
        MM_INVOKE_B( macroname, (C, a1) ), \
    MM_APPLY_COMMA_15( macroname, C, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16)

// C: pyclass instance
#define ADD_PYBIND_FIELD(C, field) C.def_readwrite(#field, &std::remove_reference<decltype(C)>::type::type::field);

#define ADD_PYBIND_FIELDS(C, ...) MM_APPLY(ADD_PYBIND_FIELD, C, __VA_ARGS__)
#define REGISTER_PYBIND_FIELDS(...) \
    template <typename Class> \
    static Class& register_fields(Class&& mod) { \
      ADD_PYBIND_FIELDS(mod, __VA_ARGS__); \
      return mod; \
    }

#define REGISTER_PYBIND \
    template <typename Class> \
    static Class& register_fields(Class&& mod) { \
      return mod; \
    }

#define PYCLASS_WITH_FIELDS(module, klass) \
  klass::register_fields(py::class_<klass>(module, #klass))
