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

#include "engine/wrapper_template.h"
#include "wrapper_callback.h"
#include "ai.h"

class GameContext {
public:
    using GC = Context;
    using Wrapper = WrapperT<WrapperCallbacks, GC::Comm, PythonOptions>;

private:
    std::unique_ptr<GC> _context;
    Wrapper _wrapper;
    int _num_frames_in_state;
    int _num_planes;

public:
    GameContext(const ContextOptions& context_options, const PythonOptions& options) {
      GameDef::GlobalInit();
      _context.reset(new GC{context_options, options});

      _num_frames_in_state = 1;
      for (const AIOptions& opt : options.ai_options) {
          _num_frames_in_state = max(_num_frames_in_state, opt.num_frames_in_state);
      }
      _num_planes = (GameDef::GetNumUnitType() + 7) * _num_frames_in_state;
    }

    void Start() {
        _context->Start(
            [&](int game_idx, const ContextOptions &context_options, const PythonOptions &options, const std::atomic_bool &done, Comm *comm) {
                    _wrapper.thread_main(game_idx, context_options, options, done, comm);
            });
    }

    std::map<std::string, int> GetParams() const {
        return std::map<std::string, int>{
            { "num_action", GameDef::GetNumAction() },
            { "num_unit_type", GameDef::GetNumUnitType() },
            { "num_planes", _num_planes },
            { "resource_dim", 2 * NUM_RES_SLOT },
            { "max_unit_cmd", _context->options().max_unit_cmd },
            { "map_x", _context->options().map_size_x },
            { "map_y", _context->options().map_size_y }
        };
    }

    CONTEXT_CALLS(GC, _context);

    EntryInfo EntryFunc(const std::string &key) {
        auto *mm = GameState::get_mm(key);
        if (mm == nullptr) return EntryInfo();

        std::string type_name = mm->type();

        const int max_unit_cmd = _context->options().max_unit_cmd;

        if (key == "s") return EntryInfo(key, type_name, { _num_planes,  _context->options().map_size_y, _context->options().map_size_x});
        else if (key == "last_r" || key == "terminal" || key == "last_terminal" || key == "id" || key == "seq" || key == "game_counter" || key == "player_id") return EntryInfo(key, type_name);
        else if (key == "pi") return EntryInfo(key, type_name, {GameDef::GetNumAction()});
        else if (key == "a" || key == "rv" || key == "V") return EntryInfo(key, type_name);
        else if (key == "res") return EntryInfo(key, type_name, {2, NUM_RES_SLOT});
        else if (key == "unit_loc") return EntryInfo(key, type_name, { max_unit_cmd, 2 });
        else if (key == "target_loc") return EntryInfo(key, type_name, { max_unit_cmd, 2 });
        else if (key == "build_type") return EntryInfo(key, type_name, { max_unit_cmd });
        else if (key == "cmd_type") return EntryInfo(key, type_name, { max_unit_cmd });

        return EntryInfo();
    }

    void Stop() {
      std::cout << "Final statistics: " << std::endl;
      std::cout << _wrapper.PrintInfo() << std::endl;
      _context.reset(nullptr); // first stop the threads, then destroy the games
    }

};

#define CONST(v) m.attr(#v) = py::int_(v)

PYBIND11_MODULE(minirts, m) {
  register_common_func<GameContext>(m);
  CONTEXT_REGISTER(GameContext)
    .def("GetParams", &GameContext::GetParams);

  // Also register other objects.
  PYCLASS_WITH_FIELDS(m, AIOptions)
    .def(py::init<>());

  PYCLASS_WITH_FIELDS(m, PythonOptions)
    .def(py::init<>())
    .def("Print", &PythonOptions::Print)
    .def("AddAIOptions", &PythonOptions::AddAIOptions);
}
