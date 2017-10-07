/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include "comm_template.h"
#include "hist.h"
#include "tree_search_options.h"
#include <pybind11/pybind11.h>

namespace py = pybind11;

template <typename GameContext>
void register_common_func(py::module &m) {
  using GC = typename GameContext::GC;
  using State = typename GC::State;
  using Infos = typename GC::Infos;

  PYCLASS_WITH_FIELDS(m, ContextOptions)
    .def(py::init<>())
    .def("print", &ContextOptions::print);

  PYCLASS_WITH_FIELDS(m, mcts::TSOptions)
    .def(py::init<>());

  PYCLASS_WITH_FIELDS(m, EntryInfo)
    .def(py::init<>());

  PYCLASS_WITH_FIELDS(m, GroupStat)
    .def(py::init<>())
    .def("info", &GroupStat::info);

  PYCLASS_WITH_FIELDS(m, Infos);

  using HistState = HistT<State>;
  PYCLASS_WITH_FIELDS(m, HistState)
    .def("newest", [](const HistState &hstate, int i) { return hstate.newest(i); })
    .def("size", &HistState::size);

  PYCLASS_WITH_FIELDS(m, MetaInfo);
}

#ifdef GIT_COMMIT_HASH
#define VERSION_SYMBOL2(hash, unstaged) inline bool version_ ## hash ## _ ## unstaged() { return true; }
#define VERSION_SYMBOL(x, y) VERSION_SYMBOL2(x, y)
    VERSION_SYMBOL(GIT_COMMIT_HASH, GIT_UNSTAGED)
#endif

#define CONTEXT_CALLS(GC, context) \
  using Infos = typename GC::Infos; \
  Infos Wait(int timeout) { return context->Wait(timeout); } \
  Infos WaitGroup(int group_id, int timeout) { return context->WaitGroup(group_id, timeout); } \
  void Steps(const Infos& infos) { context->Steps(infos); } \
  std::string Version() const { return context->Version(); } \
  void PrintSummary() const { context->PrintSummary(); } \
  GroupStat CreateGroupStat() const { return GroupStat(); } \
  int AddCollectors(int batchsize, int exclusive_id, int timeout_usec, const GroupStat &gstat) { \
    return context->comm().AddCollectors(batchsize, exclusive_id, timeout_usec, gstat); \
  } \
  int size() const { return context->size(); } \
  EntryInfo GetTensorSpec(int gid, const std::string &key, int T) { \
      return context->comm().GetCollectorGroup(gid).GetEntry(key, T, [&](const std::string &key) { return EntryFunc(key); }); \
  } \
  void AddTensor(int gid, const std::string &input_reply, const EntryInfo &e) { \
      context->comm().GetCollectorGroup(gid).AddEntry(input_reply, e); \
  } \


#define CONTEXT_REGISTER(GameContext) \
  using GC = typename GameContext::GC; \
  py::class_<GameContext>(m, #GameContext) \
    .def(py::init<const ContextOptions&, const GC::Options&>()) \
    .def("Wait", &GameContext::Wait, py::call_guard<py::gil_scoped_release>()) \
    .def("WaitGroup", &GameContext::WaitGroup, py::call_guard<py::gil_scoped_release>()) \
    .def("Steps", &GameContext::Steps, py::call_guard<py::gil_scoped_release>()) \
    .def("Version", &GameContext::Version) \
    .def("PrintSummary", &GameContext::PrintSummary) \
    .def("CreateGroupStat", &GameContext::CreateGroupStat, py::return_value_policy::copy) \
    .def("AddCollectors", &GameContext::AddCollectors) \
    .def("Start", &GameContext::Start) \
    .def("Stop", &GameContext::Stop) \
    .def("__len__", &GameContext::size) \
    .def("AddTensor", &GameContext::AddTensor) \
    .def("GetTensorSpec", &GameContext::GetTensorSpec, py::return_value_policy::copy) \

