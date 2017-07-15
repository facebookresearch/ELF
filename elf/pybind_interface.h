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
#include <pybind11/pybind11.h>

namespace py = pybind11;

template <typename GameContext>
void register_common_func(py::module &m) {
  using GC = typename GameContext::GC;
  using Options = typename GC::Options;

  PYCLASS_WITH_FIELDS(m, ContextOptions)
    .def(py::init<>())
    .def("print", &ContextOptions::print);

  PYCLASS_WITH_FIELDS(m, Options)
    .def(py::init<>());

  PYCLASS_WITH_FIELDS(m, EntryInfo)
    .def(py::init<>());

  PYCLASS_WITH_FIELDS(m, Infos);

  PYCLASS_WITH_FIELDS(m, MetaInfo);
}

#ifdef GIT_COMMIT_HASH
#define VERSION_SYMBOL2(hash, unstaged) inline bool version_ ## hash ## _ ## unstaged() { return true; }
#define VERSION_SYMBOL(x, y) VERSION_SYMBOL2(x, y)
    VERSION_SYMBOL(GIT_COMMIT_HASH, GIT_UNSTAGED)
#endif

#define CONTEXT_CALLS(GC, context) \
  Infos Wait(int timeout) { return context->Wait(timeout); } \
  Infos WaitGroup(int group_id, int timeout) { return context->WaitGroup(group_id, timeout); } \
  void Steps(const Infos& infos) { context->Steps(infos); } \
  std::string Version() const { return context->Version(); } \
  void PrintSummary() const { context->PrintSummary(); } \
  int AddCollectors(int batchsize, int hist_len) { return context->AddCollectors(batchsize, hist_len); } \
  const MetaInfo &meta(int i) const { return context->meta(i); } \
  int size() const { return context->size(); } \
\
  int CreateTensor(int gid, const std::string &key, const std::map<std::string, std::string> &desc) {\
      if (key == "input") \
         return context->GetDataAddr(gid).GetInputService().Create(desc); \
      else if (key == "reply") \
         return context->GetDataAddr(gid).GetReplyService().Create(desc); \
      else throw std::range_error("Invalid key " + key); \
  } \
  EntryInfo GetTensorInfo(int gid, const std::string &key, int k) { \
      if (key == "input") \
         return context->GetDataAddr(gid).GetInputService().entries()[k].entry_info;\
      else if (key == "reply") \
        return context->GetDataAddr(gid).GetReplyService().entries()[k].entry_info;\
      else throw std::range_error("Invalid key " + key); \
  } \
  void SetTensorAddr(int gid, const std::string &key, int k, int64_t p, int stride) { \
      if (key == "input") \
         context->GetDataAddr(gid).GetInputService().entries()[k].Set(p, stride);\
      else if (key == "reply") \
         context->GetDataAddr(gid).GetReplyService().entries()[k].Set(p, stride);\
      else throw std::range_error("Invalid key " + key); \
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
    .def("AddCollectors", &GameContext::AddCollectors) \
    .def("Start", &GameContext::Start) \
    .def("Stop", &GameContext::Stop) \
    .def("__getitem__", &GameContext::meta) \
    .def("__len__", &GameContext::size) \
    .def("CreateTensor", &GameContext::CreateTensor) \
    .def("GetTensorInfo", &GameContext::GetTensorInfo) \
    .def("SetTensorAddr", &GameContext::SetTensorAddr) \

