//File: copier.hh

#pragma once
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <string.h>

#include "lib/debugutils.hh"
#include "pybind_helper.h"
#include "shared_buffer.hh"

#define MM_DECLTYPE(_, x) decltype(x)
#define MM_STRINGIFY(_, x) #x

namespace elf_internal {

template <typename Struct>
class FieldMemoryManager {
  public:
    FieldMemoryManager(int offset) {
      _offset = offset;
    }

    // copy this field to the destination buffer
    virtual void copy_to_mem(const Struct& s, void* dst) = 0;
    virtual void copy_from_mem(const void *src, Struct& s) = 0;

    // return size of buffer required to store this field, in bytes
    virtual size_t size(const Struct& s) const = 0;

    // Return type string (can we do better?)
    virtual std::string type() const = 0;

    virtual ~FieldMemoryManager() {}

  protected:
    int _offset;
};

template <typename T>
class TypeStr;

#define TYPESTR(T) template <> class TypeStr<T> { public: static constexpr char const *str = #T; }

TYPESTR(int32_t);
TYPESTR(int64_t);
TYPESTR(char);
TYPESTR(float);
TYPESTR(unsigned char);
TYPESTR(short);
TYPESTR(bool);

template <typename Struct, typename FieldT, typename Enable=void>
class FieldMemoryManagerT;

// for POD
template <typename Struct, typename FieldT>
class FieldMemoryManagerT<Struct, FieldT,
      typename std::enable_if<std::is_pod<FieldT>::value, void>::type> : public FieldMemoryManager<Struct> {
  public:
    FieldMemoryManagerT(int offset): FieldMemoryManager<Struct>{offset} {}

    void copy_to_mem(const Struct& s, void* dst) override {
      static_assert(std::is_pod<FieldT>::value, "FieldT is not POD!");
      const FieldT* srcptr = reinterpret_cast<const FieldT*>(reinterpret_cast<const char*>(&s) + this->_offset);
      // std::cout << "src = " << *srcptr << ", dst = " << *reinterpret_cast<const FieldT*>(dst) << std::endl;
      memcpy(dst, srcptr, sizeof(FieldT));  // should work for basic type, arrays, structs
    }

    void copy_from_mem(const void *src, Struct& s) override {
      static_assert(std::is_pod<FieldT>::value, "FieldT is not POD!");
      FieldT* dstptr = reinterpret_cast<FieldT*>(reinterpret_cast<char*>(&s) + this->_offset);
      memcpy(dstptr, src, sizeof(FieldT));  // should work for basic type, arrays, structs
    }

    size_t size(const Struct&) const override { return sizeof(FieldT); }

    std::string type() const override { return std::string(TypeStr<FieldT>::str); }
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

    void copy_to_mem(const Struct& s, void* dst) override {
      const VecT* srcptr = reinterpret_cast<const VecT*>(reinterpret_cast<const char*>(&s) + this->_offset);
      memcpy(dst, srcptr->data(), srcptr->size() * sizeof(typename VecT::value_type));
    }

    void copy_from_mem(const void *src, Struct& s) override {
      // Note that we need to preallocate the size.
      VecT* dstptr = reinterpret_cast<VecT*>(reinterpret_cast<char*>(&s) + this->_offset);
      memcpy(dstptr->data(), src, dstptr->size() * sizeof(typename VecT::value_type));
    }

    size_t size(const Struct& s) const override {
      const VecT* srcptr = reinterpret_cast<const VecT*>(reinterpret_cast<const char*>(&s) + this->_offset);
      return srcptr->size() * sizeof(typename VecT::value_type);
    }

    std::string type() const override { return std::string(TypeStr<typename VecT::value_type>::str); }
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
    (void)names;
    (void)offsets;
    m_assert(names.size() == idx); m_assert(offsets.size() == idx);
  }

  map_type _field_map;
};

}

namespace elf {

template <typename State>
struct CopyItemT {
    std::string key;
    SharedBuffer buf;
    elf_internal::FieldMemoryManager<State>* mm;

    CopyItemT(const std::string &key, const SharedBuffer &buf, elf_internal::FieldMemoryManager<State> *mm)
      : key(key), buf(buf), mm(mm) {
    }

    size_t Capacity(const State &s) const {
      return buf.size() / mm->size(s);
    }

    char *ptr() const { return static_cast<char*>(buf.ptr()); }

    char *CopyToMem(const State &s, char *p) const {
      mm->copy_to_mem(s, p);
      p += mm->size(s);
      return p;
    }

    const char *CopyFromMem(State &s, const char *p) const {
      mm->copy_from_mem(p, s);
      p += mm->size(s);
      return p;
    }
};

template <typename State>
void CopyToMem(const std::vector<CopyItemT<State>> &copier, const std::vector<State *> &batch) {
  if (batch.empty()) return;
  for (const auto& item: copier) {
    m_assert(item.Check(*batch[0], batch.size()));

    char *p = item.ptr();
    for (auto* s: batch) {
       p = item.CopyToMem(*s, p);
    }
  }
}

template <typename State>
void CopyFromMem(const std::vector<CopyItemT<State>> &copier, std::vector<State *> &batch) {
  if (batch.empty()) return;
  for (const auto& item: copier) {
    m_assert(item.Check(*batch[0], batch.size()));

    const char *p = item.ptr();
    for (auto* s: batch) {
       p = item.CopyFromMem(*s, p);
    }
  }
}

}

#define DECLARE_FIELD(C, ...) \
  static elf_internal::FieldMemoryManager<C>* get_mm(std::string field_name) { \
    static auto fc = elf_internal::FieldMapConstructor<C, MM_APPLY_COMMA(MM_DECLTYPE, _, __VA_ARGS__)>( \
        {MM_APPLY_COMMA(MM_STRINGIFY, _, __VA_ARGS__)}, \
        {MM_APPLY_COMMA(offsetof, C, __VA_ARGS__)} \
        ); \
    auto& map = fc.get(); \
    auto itr = map.find(field_name); \
    if (itr == map.end()) return nullptr; \
    else return itr->second; \
  }

