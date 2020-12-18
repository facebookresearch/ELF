/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>
#include <pybind11/stl.h>

#include <string>
#include <iostream>

#include "engine/wrapper_template.h"
#include "wrapper_callback.h"
#include "ai.h"
#include "state_feature.h"

class GameContext {
public:
    using GC = Context;
    using Wrapper = WrapperT<WrapperCallbacks, GC::Comm, PythonOptions, AI>;

private:
    std::unique_ptr<GC> _context;
    Wrapper _wrapper;
    int _num_frames_in_state;

public:
    GameContext(const ContextOptions& context_options, const PythonOptions& options) {
        GameDef::GlobalInit();
        _context.reset(new GC{context_options, options});

        _num_frames_in_state = 1;
        for (const AIOptions& opt : options.ai_options) {
            _num_frames_in_state = max(_num_frames_in_state, opt.num_frames_in_state);
        }
    }

    void Start() {
        _context->Start(
            [this](int game_idx, const ContextOptions &context_options, const PythonOptions &options, const elf::Signal &signal, Comm *comm) {
                    auto params = this->GetParams();
                    this->_wrapper.thread_main(game_idx, context_options, options, signal, &params, comm);
            });
    }

    std::map<std::string, int> GetParams() const {
        return std::map<std::string, int>{
            { "num_action", GameDef::GetNumAction() },    
            { "num_unit_type", GameDef::GetNumUnitType() },   
            { "num_planes_per_time_stamp", MCExtractor::Size() },  // 22  每一个时间戳中的 planes数？
            { "num_planes", MCExtractor::Size() * _num_frames_in_state },  //  22 每一个状态包含一帧的数据
            { "resource_dim", 2 * NUM_RES_SLOT }, // 10
            { "max_unit_cmd", _context->options().max_unit_cmd }, // 1
            { "map_x", _context->options().map_size_x }, // 20
            { "map_y", _context->options().map_size_y }, // 20  
            { "num_cmd_type", CmdInput::CI_NUM_CMDS },   // 4
            { "reduced_dim", MCExtractor::Size() * 5 * 5 }// 22*5*5
        };
    }

    CONTEXT_CALLS(GC, _context);

    EntryInfo EntryFunc(const std::string &key) {
        auto *mm = GameState::get_mm(key);
        if (mm == nullptr) return EntryInfo();

        std::string type_name = mm->type();
        const int mapx = _context->options().map_size_x;
        const int mapy = _context->options().map_size_y;
        const int max_unit_cmd = _context->options().max_unit_cmd;
        const int reduced_size = MCExtractor::Size() * 5 * 5;

        if (key == "s") return EntryInfo(key, type_name, { (int)MCExtractor::Size() * _num_frames_in_state,  _context->options().map_size_y, _context->options().map_size_x});
        else if (key == "last_r" || key == "terminal" || key == "last_terminal" || key == "id" || key == "seq" || key == "game_counter" || key == "player_id") return EntryInfo(key, type_name);
        else if (key == "pi") return EntryInfo(key, type_name, {GameDef::GetNumAction()});
        else if (key == "a" || key == "rv" || key == "V" || key == "action_type") return EntryInfo(key, type_name);
        else if (key == "uloc") return EntryInfo(key, type_name, { max_unit_cmd });
        else if (key == "tloc") return EntryInfo(key, type_name, { max_unit_cmd });
        else if (key == "bt") return EntryInfo(key, type_name, { max_unit_cmd });
        else if (key == "ct") return EntryInfo(key, type_name, { max_unit_cmd });
        else if (key == "uloc_prob") return EntryInfo(key, type_name, { max_unit_cmd,  mapx * mapy });
        else if (key == "tloc_prob") return EntryInfo(key, type_name, { max_unit_cmd, mapx * mapy });
        else if (key == "bt_prob") return EntryInfo(key, type_name, { max_unit_cmd, GameDef::GetNumUnitType() });
        else if (key == "ct_prob") return EntryInfo(key, type_name, { max_unit_cmd, CmdInput::CI_NUM_CMDS });
        else if (key == "reduced_s") return EntryInfo(key, type_name, { reduced_size });
        else if (key == "reduced_next_s") return EntryInfo(key, type_name, { reduced_size });

        return EntryInfo();
    }

    void ApplyExtractorParams(const MCExtractorOptions &opt) {
        std::cout << opt.info() << std::endl;
        MCExtractor::Init(opt);
    }

    void ApplyExtractorUsage(const MCExtractorUsageOptions &opt) {
        std::cout << opt.info() << std::endl;
        MCExtractor::InitUsage(opt);
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
    .def("GetParams", &GameContext::GetParams)
    .def("ApplyExtractorParams", &GameContext::ApplyExtractorParams)
    .def("ApplyExtractorUsage", &GameContext::ApplyExtractorUsage);

  // Also register other objects.
  PYCLASS_WITH_FIELDS(m, AIOptions)
    .def(py::init<>());

  PYCLASS_WITH_FIELDS(m, GameState)
    .def("AddUnitCmd", &GameState::AddUnitCmd);

  PYCLASS_WITH_FIELDS(m, PythonOptions)
    .def(py::init<>())
    .def("Print", &PythonOptions::Print)
    .def("AddAIOptions", &PythonOptions::AddAIOptions);

  PYCLASS_WITH_FIELDS(m, MCExtractorOptions)
    .def(py::init<>());

  PYCLASS_WITH_FIELDS(m, MCExtractorUsageOptions)
    .def(py::init<>())
    .def("Set", &MCExtractorUsageOptions::Set);
}
