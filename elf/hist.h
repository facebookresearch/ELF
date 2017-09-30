#pragma once

#include <utility>
#include "pybind_helper.h"
#include "python_options_utils_cpp.h"

#include "circular_queue.h"
#include "copier.hh"
#include "common.h"

template <typename _Data>
class HistT {
public:
    using Data = _Data;
    using State = typename Data::State;
    using DataPrepareReturn = decltype(std::declval<Data>().Prepare(SeqInfo()));

private:
    std::unique_ptr<CircularQueue<Data>> _h;

public:
    HistT() { } 
    HistT(const HistT<Data> &other) {
        _h.reset(new CircularQueue<Data>(*other._h));
    }
    void InitHist(int len) {
        _h.reset(new CircularQueue<Data>(len));
    }

    void Restart() {
        // Cannot clear history, For game whose reward is only revealed in the end,
        // clear history will lead to even sparser reward (since for a long trajectory,
        // the state with the reward will appear only in the end).
        // _h->clear();
    }

    DataPrepareReturn Prepare(const SeqInfo &seq) {
        // we move the history forward.
        return _h->GetRoom().Prepare(seq);
    }

    // Note that these two will be called after .Push, so we need to retrieve them from .newest().
    // TODO: Make them consistent.
    int size() const { return _h->size(); }
    std::vector<Data> &v() { return _h->v(); }
    const std::vector<Data> &v() const { return _h->v(); }

    // oldest = 0, newest = _h->maxlen() - 1
    Data &oldest(int i = 0) { return _h->get_from_push(_h->maxlen() - i - 1); }
    const Data &oldest(int i = 0) const { return _h->get_from_push(_h->maxlen() - i - 1); }
    Data &newest(int i = 0) {
      // std::cout << "[" <<  _h->get_from_push(i).id << "] newest(" << i << ")" << std::endl;
      return _h->get_from_push(i);
    }
    const Data &newest(int i = 0) const { return _h->get_from_push(i); }

    REGISTER_PYBIND;
};

namespace elf {

template <typename State>
void CopyToMem(const std::vector<CopyItemT<State>> &copier, const std::vector<HistT<State> *> &batch) {
  if (batch.empty()) return;
  size_t batchsize = batch.size();
  size_t overall_hist_len = batch[0]->size();

  for (const auto& item: copier) {
    size_t capacity = item.Capacity(batch[0]->newest());
    size_t hist_len = capacity / batchsize;
    size_t min_hist_len = std::min(hist_len, overall_hist_len);

    char *p = item.ptr();
    // std::cout << "key = " << item.key << ". p = " << std::hex << (void *)p << std::dec << " min_hist_len = " << min_hist_len << std::endl;
    //
    for (size_t t = 0; t < min_hist_len; ++t) {
      for (auto* s: batch) {
        const State &state = s->newest(min_hist_len - t - 1);
        p = item.CopyToMem(state, p);
      }
    }
    if (hist_len > overall_hist_len) {
      // Fill them with the oldest hist.
      for (size_t i = overall_hist_len; i < hist_len; ++i) {
        for (auto *s: batch) {
          const State &state = s->newest(min_hist_len - 1);
          p = item.CopyToMem(state, p);
        }
      }
    }
  }
}

template <typename State>
void CopyFromMem(const std::vector<CopyItemT<State>> &copier, std::vector<HistT<State> *> &batch) {
  if (batch.empty()) return;
  size_t batchsize = batch.size();
  size_t overall_hist_len = batch[0]->size();

  for (const auto& item: copier) {
    size_t capacity = item.Capacity(batch[0]->newest());
    size_t hist_len = capacity / batchsize;
    size_t min_hist_len = std::min(hist_len, overall_hist_len);

    const char *p = item.ptr();
    for (size_t t = 0; t < min_hist_len; ++t) {
      for (auto* s: batch) {
         State &state = s->newest(min_hist_len - t - 1);
         p = item.CopyFromMem(state, p);
       }
    }
  }
}

}
