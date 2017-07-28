#pragma once

#include <utility>

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
    void InitHist(int len) {
        _h.reset(new CircularQueue<Data>(len));
    }

    DataPrepareReturn Prepare(const SeqInfo &seq) {
        // we move the history forward.
        if (_h->full()) _h->Pop();
        _h->Push();
        return newest().Prepare(seq);
    }

    // Note that these two will be called after .Push, so we need to retrieve them from .newest().
    // TODO: Make them consistent.
    int size() const { return _h->size(); }
    std::vector<Data> &v() { return _h->v(); }
    const std::vector<Data> &v() const { return _h->v(); }

    // oldest = 0, newest = _h->maxlen() - 1
    Data &oldest(int i = 0) { return _h->get_from_push(_h->maxlen() - i - 1); }
    const Data &oldest(int i = 0) const { return _h->get_from_push(_h->maxlen() - i - 1); }
    Data &newest(int i = 0) { return _h->get_from_push(i); }
    const Data &newest(int i = 0) const { return _h->get_from_push(i); }
};

namespace elf {

template <typename State>
void CopyToMem(const std::vector<CopyItemT<State>> &copier, const std::vector<HistT<State> *> &batch) {
  if (batch.empty()) return;
  size_t batchsize = batch.size();

  for (const auto& item: copier) {
    size_t capacity = item.Capacity(batch[0]->newest());
    size_t hist_len = capacity / batchsize;

    char *p = item.ptr();
    for (auto* s: batch) {
       for (size_t t = 0; t < hist_len; ++t) {
         p = item.CopyToMem(s->newest(t), p);
         m_assert(p != nullptr);
       }
    }
  }
}

template <typename State>
void CopyFromMem(const std::vector<CopyItemT<State>> &copier, std::vector<HistT<State> *> &batch) {
  if (batch.empty()) return;
  size_t batchsize = batch.size();

  for (const auto& item: copier) {
    size_t capacity = item.Capacity(batch[0]->newest());
    size_t hist_len = capacity / batchsize;
    // [batchsize, histsize, xx, xx, xx]

    const char *p = item.ptr();
    for (auto* s: batch) {
       for (size_t t = 0; t < hist_len; ++t) {
         p = item.CopyFromMem(s->newest(t), p);
         m_assert(p != nullptr);
       }
    }
  }
}

}
