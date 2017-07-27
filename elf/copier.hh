//File: copier.hh

#pragma once
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <string.h>

#include "lib/debugutils.hh"
#include "pybind_helper.h"
#include "shared_buffer.hh"


namespace {

template <typename Struct>
class FieldMemoryManager {
  public:
    FieldMemoryManager(int offset) {
      _offset = offset;
    }

    // copy this field to the destination buffer
    virtual void copy_to(const Struct& s, void* dst) = 0;

    // return size of buffer required to store this field, in bytes
    virtual size_t size(const Struct& s) = 0;

    virtual ~FieldMemoryManager() {}

  protected:
    int _offset;
};

template <typename Struct, typename FieldT, typename Enable=void>
class FieldMemoryManagerT;

// for POD
template <typename Struct, typename FieldT>
class FieldMemoryManagerT<Struct, FieldT,
      typename std::enable_if<std::is_pod<FieldT>::value, void>::type> : public FieldMemoryManager<Struct> {
  public:
    FieldMemoryManagerT(int offset): FieldMemoryManager<Struct>{offset} {}

    void copy_to(const Struct& s, void* dst) override {
      static_assert(std::is_pod<FieldT>::value, "FieldT is not POD!");
      const FieldT* srcptr = reinterpret_cast<const FieldT*>(reinterpret_cast<const char*>(&s) + this->_offset);
      memcpy(dst, srcptr, sizeof(FieldT));  // should work for basic type, arrays, structs
    }

    size_t size(const Struct&) override { return sizeof(FieldT); }
};


template<typename T> struct is_pod_vector { static constexpr bool value = false; };
template<template<typename...> class C, typename U>
struct is_pod_vector<C<U>> {
    static constexpr bool value = std::is_same<C<U>, std::vector<U>>::value && std::is_pod<U>::value;
};

template <typename Struct, typename VecT>
class FieldMemoryManagerT<Struct, VecT,
      typename std::enable_if<is_pod_vector<VecT>::value, void>::type> : public FieldMemoryManager<Struct> {
  public:
    FieldMemoryManagerT(int offset): FieldMemoryManager<Struct>{offset} {}

    void copy_to(const Struct& s, void* dst) override {
      const VecT* srcptr = reinterpret_cast<const VecT*>(reinterpret_cast<const char*>(&s) + this->_offset);
      memcpy(dst, srcptr->data(), srcptr->size() * sizeof(typename VecT::value_type));
    }

    size_t size(const Struct& s) override {
      const VecT* srcptr = reinterpret_cast<const VecT*>(reinterpret_cast<const char*>(&s) + this->_offset);
      return srcptr->size() * sizeof(typename VecT::value_type);
    }
};

template <typename State, typename ...Ts>
struct FieldMapConstructor {
  using map_type = std::unordered_map<std::string, FieldMemoryManager<State>*>;

  FieldMapConstructor(
      const std::vector<std::string>& names,
      const std::vector<int>& offsets) {
    m_assert(names.size() == sizeof...(Ts));
    m_assert(offsets.size() == sizeof...(Ts));
    _add_map<0, Ts...>(names, offsets);
  }

  const map_type& get() const
  { return _field_map; }

  ~FieldMapConstructor() {
    for (auto& pair: _field_map)
      delete pair.second;
  }

private:
  template <size_t idx, typename T, typename ... TTs>
  void _add_map(
      const std::vector<std::string>& names,
      const std::vector<int>& offsets) {
    this->_field_map[names[idx]] = new FieldMemoryManagerT<State, T>(offsets[idx]);
    _add_map<idx+1, TTs...>(names, offsets);
  }

  template <size_t idx>
  void _add_map(
      const std::vector<std::string>& names,
      const std::vector<int>& offsets) {
    m_assert(names.size() == idx); m_assert(offsets.size() == idx);
  }

  map_type _field_map;
};

}

namespace elf {

template <typename State>
class Copier {
  using buffermap_t = std::unordered_map<std::string, SharedBuffer>;
  public:
    Copier(const std::unordered_map<std::string, SharedBuffer>& buffermap) {
      static_assert(std::is_standard_layout<State>::value, "Copier only works on classes with standard layout!");
      for (auto& pair: buffermap)
        fieldmap_.emplace_back(
            std::make_tuple(pair.first, pair.second, State::get_mm(pair.first)));
    }

    // copy a single instance
    void copy(const State& s) {
      for (auto& tp: fieldmap_) {
        auto& mm = std::get<2>(tp);
        size_t sz = mm->size(s);
        auto& buf = std::get<1>(tp);
        m_assert(buf.size() == sz);
        mm->copy_to(s, buf.ptr());
      }
    }

    void copy_batch(const std::vector<State>& states) {
      for (auto& tp: fieldmap_) {
        auto& mm = std::get<2>(tp);
        auto& buf = std::get<1>(tp);
        char* ptr = static_cast<char*>(buf.ptr());

        m_assert(states.size() > 0);
        size_t single_size = mm->size(states[0]);
        size_t total_size = single_size * states.size();
        m_assert(total_size == buf.size());

        for (auto& s: states) {
          size_t sz = mm->size(s);
          m_assert(single_size == sz);
          mm->copy_to(s, ptr);
          ptr += single_size;
        }
      }
    }

  private:
    std::vector<std::tuple<std::string, SharedBuffer, FieldMemoryManager<State>*>> fieldmap_;
};

}

#define MM_DECLTYPE(_, x) decltype(x)
#define MM_STRINGIFY(_, x) #x
#define DECLARE_FIELD(C, ...) \
  static FieldMemoryManager<C>* get_mm(std::string field_name) { \
    static auto fc = FieldMapConstructor<C, MM_APPLY_COMMA(MM_DECLTYPE, _, __VA_ARGS__)>( \
        {MM_APPLY_COMMA(MM_STRINGIFY, _, __VA_ARGS__)}, \
        {MM_APPLY_COMMA(offsetof, C, __VA_ARGS__)} \
        ); \
    auto& map = fc.get(); \
    auto itr = map.find(field_name); \
    m_assert(itr != map.end()); \
    return itr->second; \
  }
