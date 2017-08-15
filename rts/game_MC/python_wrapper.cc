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

#include "../engine/wrapper_template.h"
#include "wrapper_callback.h"
#include "ai.h"

class GameContext {
public:
    using GC = Context;

private:
    int _T;
    std::unique_ptr<GC> _context;

public:
    GameContext(const ContextOptions& context_options, const PythonOptions& options) {
      _T = context_options.T;
      // Initialize enums.
      init_enums();
      WrapperCallbacks::GlobalInit();

      _context.reset(new GC{context_options, options});
    }

    void Start() {
        auto data_init = [&](int game_idx, HistT<GameState> &hstate) {
            hstate.InitHist(_T);
            for (auto &item : hstate.v()) {
                item.Init(game_idx, GameDef::GetNumAction());
            }
        };
        _context->Start(data_init, thread_main<WrapperCallbacks, GC::AIComm, PythonOptions>);
    }

    const std::string &game_unittype2str(int unit_type) const {
        return _UnitType2string((UnitType)unit_type);
    }
    const std::string &game_aistate2str(int ai_state) const {
        return _AIState2string((AIState)ai_state);
    }

    std::map<std::string, int> GetParams() const {
        return std::map<std::string, int>{
            { "num_action", GameDef::GetNumAction() },
            { "num_unit_type", GameDef::GetNumUnitType() },
            { "resource_dim", 2 * NUM_RES_SLOT }
        };
    }

    CONTEXT_CALLS(GC, _context);

    EntryInfo EntryFunc(const std::string &key) {
        auto *mm = GameState::get_mm(key);
        if (mm == nullptr) return EntryInfo();

        std::string type_name = mm->type();

        if (key == "s") return EntryInfo(key, type_name, {GameDef::GetNumUnitType() + 7, 20, 20});
        else if (key == "last_r" || key == "terminal" || key == "last_terminal" || key == "id" || key == "seq" || key == "game_counter") return EntryInfo(key, type_name);
        else if (key == "pi") return EntryInfo(key, type_name, {GameDef::GetNumAction()});
        else if (key == "a" || key == "rv" || key == "V") return EntryInfo(key, type_name);
        else if (key == "res") return EntryInfo(key, type_name, {2, NUM_RES_SLOT});

        return EntryInfo();
    }

    void Stop() {
      _context.reset(nullptr); // first stop the threads, then destroy the games
    }

};

#define CONST(v) m.attr(#v) = py::int_(v)

PYBIND11_PLUGIN(minirts) {
  py::module m("minirts", "MiniRTS game");
  register_common_func<GameContext>(m);
  CONTEXT_REGISTER(GameContext)
    .def("GetParams", &GameContext::GetParams);

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
  CONST(AI_TD_BUILT_IN);
  CONST(AI_TD_NN);
  CONST(ACTION_GLOBAL);
  CONST(ACTION_PROB);
  CONST(ACTION_REGIONAL);

  return m.ptr();
}
