/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include "pybind_helper.h"

template <typename T>
inline std::string type_desc() { return "unknown"; }

#define TYPE_DESC(type) \
template <> \
inline std::string type_desc<type>() { return #type; }

TYPE_DESC(int);
TYPE_DESC(float);
TYPE_DESC(double);
TYPE_DESC(int64_t);
TYPE_DESC(unsigned char);

template <typename AIComm>
class FieldBase {
protected:
    int _hist_loc;

public:
    void SetHistLoc(int hist_loc) { _hist_loc = hist_loc; }

    virtual bool valid() const = 0;
    virtual std::string desc() const = 0;

    virtual void Set(int64_t p, int stride) = 0;
    // From AIComm to Ptr
    virtual void ToPtr(int batch_idx, const AIComm& ai_comm) {
        (void)batch_idx;
        (void)ai_comm;
        throw std::range_error("ToPtr() is not implemented!");
    }

    // From Ptr to AIComm
    virtual void FromPtr(int batch_idx, AIComm& ai_comm) const {
        (void)batch_idx;
        (void)ai_comm;
        throw std::range_error("FromPtr() is not implemented!");
    }
    virtual std::string info() const = 0;
};

template <typename AIComm, typename T>
class FieldT : public FieldBase<AIComm> {
protected:
    T *_p;
    int _stride;

    T *addr(int idx) { return _p + idx * _stride; }
    const T *addr(int idx) const { return _p + idx * _stride; }

public:
    using value_type = T;

    FieldT() : _p(nullptr), _stride(0) { }
    void Set(int64_t p, int stride) override {
        _p = reinterpret_cast<T *>(p);
        _stride = stride;
    }

    bool valid() const override { return _p != nullptr && _stride > 0; }

    // Get element size.
    std::string desc() const override { return type_desc<T>(); }

    std::string info() const override {
      std::stringstream ss;
      ss << std::hex << (void *)_p << std::dec << " [" << _stride << "] (" << sizeof(T) << ")";
      return ss.str();
    }
};

#define FIELD_SIMPLE(comm_name, name, type, subfield) \
class Field ## name : public FieldT<comm_name, type> { \
public: \
    void ToPtr(int batch_idx, const comm_name& ai_comm) { \
      *this->addr(batch_idx) = ai_comm.newest(this->_hist_loc).subfield;\
    } \
    void FromPtr(int batch_idx, comm_name& ai_comm) const {\
      ai_comm.newest(this->_hist_loc).subfield = *this->addr(batch_idx);\
    } \
}


template<typename AIComm>
FIELD_SIMPLE(AIComm, Seq, int, seq);

template<typename AIComm>
FIELD_SIMPLE(AIComm, ReplyVersion, int, reply_version);

template<typename AIComm>
class FieldId : public FieldT<AIComm, int> {
public:
    void ToPtr(int batch_idx, const AIComm& ai_comm) override {
      *this->addr(batch_idx) = ai_comm.GetMeta().id;
    }
};

using SizeType = std::vector<int>;

struct EntryInfo {
  std::string key;
  int hist_loc_for_py;
  std::string type;
  SizeType sz;

  REGISTER_PYBIND_FIELDS(key, hist_loc_for_py, type, sz);
};

template <typename AIComm>
struct AddrEntryT {
    using AddrType = std::unique_ptr<FieldBase<AIComm>>;

    EntryInfo entry_info;
    AddrType addr;

    std::string info() const {
        std::stringstream ss;
        ss << "[T=" << entry_info.hist_loc_for_py << "]:" << entry_info.key << ": "
           << addr->info() << std::endl;
        return ss.str();
    }

    void Set(int64_t p, int stride) { addr->Set(p, stride); }
};

// DataAddr service
template <typename AIComm>
class DataAddrServiceT {
public:
    using AddrEntry = AddrEntryT<AIComm>;
    using AddrType = typename AddrEntry::AddrType;
    using DescType = std::map<std::string, std::string>;
    using CustomFunc = std::function<bool (int batchsize, const std::string& key, const std::string& v, SizeType *, FieldBase<AIComm> **)>;

private:
    std::vector<AddrEntry> _entries;
    CustomFunc _func = (CustomFunc)nullptr;

public:
    void RegCustomFunc(CustomFunc func) { _func = func; }

    int Create(const DescType &desc) {
        _entries.clear();

        // Check special entries.
        int T = 1;
        {
            auto it = desc.find("_T");
            if (it != desc.end()) T = stoi(it->second);
        }

        int batchsize = 1;
        {
            auto it = desc.find("_batchsize");
            if (it != desc.end()) batchsize = stoi(it->second);
        }

        // Duplicate with T
        for (int t = 0; t < T; ++t) {
            for (auto it = desc.begin(); it != desc.end(); ++it) {
                const std::string &key = it->first;
                if (key[0] == '_') continue;

                SizeType sz;
                FieldBase<AIComm> *p = nullptr;

                if (key == "seq") {
                    sz = SizeType{batchsize};
                    p = new FieldSeq<AIComm>();
                } else if (key == "id") {
                    sz = SizeType{batchsize};
                    p = new FieldId<AIComm>();
                } else if (key == "rv") {
                    // reply version
                    sz = SizeType{batchsize};
                    p = new FieldReplyVersion<AIComm>();
                } else if (_func != nullptr) {
                    if (! _func(batchsize, key, it->second, &sz, &p))
                      continue;
                }

                if (p == nullptr) throw std::range_error("Unknown key in Create(): "  + key);
                p->SetHistLoc(t);

                _entries.emplace_back();
                _entries.back().entry_info.key = key;
                _entries.back().entry_info.hist_loc_for_py = T - t - 1;
                _entries.back().entry_info.sz = sz;
                _entries.back().entry_info.type = p->desc();
                _entries.back().addr.reset(p);
            }
        }
        return _entries.size();
    }

    std::string info() const {
        std::stringstream ss;
        for (const auto &entry : _entries) {
            ss << entry.info();
        }
        return ss.str();
    }

    std::vector<AddrEntry> &entries() { return _entries; }
    const std::vector<AddrEntry> &entries() const { return _entries; }
};

// Finally DataAddr
template <typename AIComm>
class DataAddrT {
public:
    using DataAddrService = DataAddrServiceT<AIComm>;
    using CustomFunc = typename DataAddrService::CustomFunc;
    using AddrEntry = typename DataAddrService::AddrEntry;
    using AddrType = typename AddrEntry::AddrType;
    using CustomFieldFunc = typename DataAddrService::CustomFunc;

private:
    DataAddrService _input_addrs;
    DataAddrService _reply_addrs;

public:
    DataAddrService &GetInputService() { return _input_addrs; }
    DataAddrService &GetReplyService() { return _reply_addrs; }

    void RegCustomFunc(CustomFunc func) {
        _input_addrs.RegCustomFunc(func);
        _reply_addrs.RegCustomFunc(func);
    }

    void GetInput(int batch_idx, const AIComm& ai_comm) {
        // Copy stuff to input
        for (auto &entry : _input_addrs.entries()) {
            if (entry.addr->valid()) entry.addr->ToPtr(batch_idx, ai_comm);
        }
    }

    void PutReply(int batch_idx, AIComm &ai_comm) {
        for (const auto &entry : _reply_addrs.entries()) {
            if (entry.addr->valid()) entry.addr->FromPtr(batch_idx, ai_comm);
        }
    }
};
