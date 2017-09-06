/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

//File: python_wrapper.cc
//Author: Yuxin Wu <ppwwyyxxc@gmail.com>

#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>
#include <pybind11/stl.h>

#include "../elf/pybind_helper.h"

#include "game_context.h"

namespace py = pybind11;

PYBIND11_MODULE(go_game, m) {
  register_common_func<GameContext>(m);

  CONTEXT_REGISTER(GameContext)
      .def("GetParams", &GameContext::GetParams)
      .def("ShowBoard", &GameContext::ShowBoard);

  // Also register other objects.
  PYCLASS_WITH_FIELDS(m, GameOptions)
    .def(py::init<>());

}
