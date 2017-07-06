/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>
#include <pybind11/stl.h>

#include <string>
#include <iostream>

#include "../engine/fields.h"
#include "../engine/wrapper_template.h"
#include "wrapper_callback.h"

using GC = ContextT<PythonOptions, ExtGame, Reply>;

class GameContext {
public:
    using GC = ContextT<PythonOptions, ExtGame, Reply>;

private:
    std::unique_ptr<GC> _context;

public:
    GameContext(const ContextOptions& context_options, const PythonOptions& options) {
      // Initialize enums.
      init_enums();
      WrapperCallbacks::GlobalInit();

      _context.reset(new GC{context_options, options, CustomFieldFunc});
    }

    void Start() {
        _context->Start(thread_main<WrapperCallbacks, GC::AIComm>);
    }

    const std::string &game_unittype2str(int unit_type) const {
        return _UnitType2string((UnitType)unit_type);
    }
    const std::string &game_aistate2str(int ai_state) const {
        return _AIState2string((AIState)ai_state);
    }

    int get_num_actions() const { return GameDef::GetNumAction(); }
    int get_num_unittype() const { return GameDef::GetNumUnitType(); }

    void Stop() {
      _context.reset(nullptr); // first stop the threads, then destroy the games
    }

    CONTEXT_CALLS(GC, _context);
};

#define CONST(v) m.attr(#v) = py::int_(v)

PYBIND11_PLUGIN(minirts) {
  py::module m("minirts", "MiniRTS game");
  register_common_func<GameContext>(m);
  CONTEXT_REGISTER(GameContext)
    .def("get_num_actions", &GameContext::get_num_actions)
    .def("get_num_unittype", &GameContext::get_num_unittype);

  // Define symbols.
  CONST(ST_INVALID);
  CONST(ST_NORMAL);
  CONST(AI_INVALID);
  CONST(AI_SIMPLE);
  CONST(AI_HIT_AND_RUN);
  CONST(AI_NN);
  CONST(AI_MCTS_VALUE);
  CONST(AI_FLAG_NN);
  CONST(AI_FLAG_SIMPLE);
  CONST(AI_TD_BULIT_IN);
  CONST(AI_TD_NN);
  CONST(ACTION_GLOBAL);
  CONST(ACTION_PROB);
  CONST(ACTION_REGIONAL);

  return m.ptr();
}
