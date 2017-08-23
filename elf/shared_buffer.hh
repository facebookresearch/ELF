//File: shared_buffer.hh

#pragma once

#include <functional>
#include <vector>
#include <numeric>
#include <deque>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace elf {

// A buffer owned outside of C++
class SharedBuffer {
  public:
    SharedBuffer() = default;
    SharedBuffer(const SharedBuffer&) = default;

    // size is the number of Ts in the buffer
    SharedBuffer(void* buf, std::size_t size):
      _buf{buf}, _size{size} {}

    SharedBuffer(std::uint64_t ptr, std::size_t size):
      _buf{reinterpret_cast<void*>(ptr)}, _size(size) {
        static_assert(
          sizeof(void*) == sizeof(std::uint64_t), "Only supports 64bit system!");
      }

    std::size_t size() const { return _size; }

    void* ptr() const { return _buf; }

  private:
    void* _buf;
    size_t _size;
};


template <typename T>
class SharedArray {
  public:
    SharedArray() = default;
    SharedArray(const SharedArray&) = default;
    SharedArray& operator =(const SharedArray&) = default;
    SharedArray(SharedArray&&) = default;

    SharedArray(T* buf, const std::vector<int>& shapes):
      _buf{buf, static_cast<size_t>(std::accumulate(std::begin(shapes), std::end(shapes), 1, std::multiplies<int>())) * sizeof(T)},
      _shapes{shapes} { _compute_strides(); }

    SharedArray(std::uint64_t ptr, const std::vector<int>& shapes):
      _buf{ptr, static_cast<size_t>(std::accumulate(std::begin(shapes), std::end(shapes), 1, std::multiplies<int>())) * sizeof(T)},
      _shapes{shapes} { _compute_strides(); }

    template <typename... Ix>
    T* ptr(Ix... index) {
      return static_cast<T*>(_buf.ptr()) + _offset(_strides, index...);
    }

    template <typename... Ix>
    T& operator [](Ix... index) { return *(ptr(index...)); }

    template <typename... Ix>
    const T& operator [](Ix... index) const { return *(ptr(index...)); }

  private:
    template <size_t Dim = 0>
    size_t _offset(const std::vector<int>&) { return 0; }
    template <size_t Dim = 0, typename... Ix>
    size_t _offset(const std::vector<int>& strides, size_t i, Ix... index) {
      return i * strides[Dim] + _offset<Dim+1>(strides, index...);
    }

    void _compute_strides() {
      auto ndim = _shapes.size();
      if (ndim) {
        _strides.resize(ndim, 1);
        for (size_t i = 0; i < ndim - 1; i++)
          for (size_t j = 0; j < ndim - 1 - i; j++)
            _strides[j] *= _shapes[ndim - 1 - i];
      }
    }

    SharedBuffer _buf;
    std::vector<int> _shapes;
    std::vector<int> _strides;
};


// TODO use circular buffer
template <typename T>
class HistoryBuffer {
  public:
    explicit HistoryBuffer(int max_len):
      _max_len{max_len} {}

    template< class... Args >
      inline void emplace_back(Args&&... args) {
        _q.emplace_back(std::forward<Args...>(args...));
        if (_q.size() > _max_len) _q.pop_front();
      }

    inline void emplace_back() {
      _q.emplace_back();
      if (_q.size() > _max_len) _q.pop_front();
    }

    // [0] is the oldest
    const T& operator[] (int idx) const {
      return _q[idx];
    }

    T& back() { return _q.back(); }

    size_t size() const { return _q.size(); }

    void clear() { _q.clear(); }

  private:
    int _max_len;
    std::deque<T> _q;
};

} // namespace elf

