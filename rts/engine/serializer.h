/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#ifndef _SERIALIZER_H_
#define _SERIALIZER_H_

#include <iomanip>
#include <iostream>
#include <functional>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <queue>
#include <utility>
#include <unordered_map>
#include <memory>
#include "pq_extend.h"

namespace serializer {

class saver {
private:
    std::stringstream _oo;
    bool _binary;

public:
    explicit saver(bool isbinary): _binary(isbinary) {}
    bool write_to_file(const std::string &s) {
        std::ofstream oFile(s, _binary ? std::ios::binary | std::ios::out : std::ios::out);
        if (oFile.is_open()) {
            oFile << _oo.rdbuf();
            return true;
        }
        return false;
    }
    std::string get_str() {return _oo.str();}
    std::stringstream &get() { return _oo; }
    bool is_binary() const { return _binary; }

    friend saver &operator<<(saver &s, const int& v) {
        if (s.is_binary()) {
            s.get().write(reinterpret_cast<const char *>(&v), sizeof(int));
        } else {
            s.get() << v;
        }
        return s;
    }

    friend saver &operator<<(saver &s, const uint64_t& v) {
        if (s.is_binary()) {
            s.get().write(reinterpret_cast<const char *>(&v), sizeof(uint64_t));
        } else {
            s.get() << v;
        }
        return s;
    }

    friend saver &operator<<(saver &s, const float& v) {
        if (s.is_binary()) {
            s.get().write(reinterpret_cast<const char *>(&v), sizeof(float));
        } else {
            s.get() << std::setprecision(20) << v;
        }
        return s;
    }

    friend saver &operator<<(saver &s, const bool& v) {
        if (s.is_binary()) {
            s.get().write(reinterpret_cast<const char *>(&v), sizeof(bool));
        } else {
            s.get() << v;
        }
        return s;
    }

    template <typename T>
    friend saver &operator<<(saver &s, const std::unique_ptr<T>& v) {
        s << *v;
        return s;
    }

    template <typename T>
    friend saver &operator<<(saver &s, const p_queue<T>& v) {
        const T *p = &v.top();
        int size = v.size();
        if (! s.is_binary()) s.get() << " ";
        s << size;
        if (! s.is_binary()) s.get() << "\n";
        for (unsigned int i = 0; i < v.size(); ++i) {
            s << p[i];
            if (! s.is_binary()) s.get() << "\n";
        }
        return s;
    }

    template <typename T1, typename T2>
    friend saver &operator<<(saver &s, const std::pair<T1, T2>& v) {
        if (! s.is_binary()) s.get() << " ";
        s << v.first;
        if (! s.is_binary()) s.get() << " ";
        s << v.second;
        if (! s.is_binary()) s.get() << " ";
        return s;
    }

    friend saver &operator<<(saver &s, const std::string& v) {
        if (s.is_binary()) {
            int size = v.size();
            s << size;
            s.get().write(v.c_str(), v.size());
        } else {
            s.get() << "\"" << v << "\"";
        }
        return s;
    }

    template <typename T>
    friend saver &operator<<(saver &s, const std::vector<T>& v) {
        int size = v.size();
        if (! s.is_binary()) s.get() << " ";
        s << size;
        if (! s.is_binary()) s.get() << "\n";
        for (int i = 0; i < size; ++i) {
            s << v[i];
            if (! s.is_binary()) s.get() << "\n";
        }
        return s;
    }

    template <typename Key, typename T>
    friend saver &operator<<(saver &s, const std::map<Key, T>& m) {
        int size = m.size();
        if (! s.is_binary()) s.get() << " ";
        s << size;
        if (! s.is_binary()) s.get() << "\n";
        for (const auto& item : m) {
            s << item;
            if (! s.is_binary()) s.get() << "\n";
        }
        return s;
    }

    template <typename Key, typename T>
    friend saver &operator<<(saver &s, const std::unordered_map<Key, T>& m) {
        int size = m.size();
        if (! s.is_binary()) s.get() << " ";
        s << size;
        if (! s.is_binary()) s.get() << "\n";
        for (const auto& item : m) {
            s << item;
            if (! s.is_binary()) s.get() << "\n";
        }
        return s;
    }
};

/*
template<typename T>
saver &_save_plain<T, true>::save(saver &s, const T &v) {
    if (s.is_binary()) {
        s.get().write(reinterpret_cast<const char *>(&v), sizeof(T));
    } else {
        s.get() << v << " ";
    }
    return s;
}
*/

class loader {
private:
    std::stringstream _ii;
    bool _binary;

public:
    explicit loader(bool isbinary) : _binary(isbinary) {}
    bool read_from_file(const std::string &s) {
        std::ifstream iFile(s, _binary ? (std::ios::binary | std::ios::in) : std::ios::in);
        if (iFile.is_open()) {
            _ii << iFile.rdbuf();
            return true;
        }
        return false;
    }
    void set_str(const std::string &s) {_ii << s;}
    std::stringstream &get() { return _ii; }
    bool is_binary() const { return _binary; }

    friend loader &operator>>(loader &l, int& v) {
        if (l.is_binary()) {
            l.get().read(reinterpret_cast<char *>(&v), sizeof(int));
        } else {
            l.get() >> v;
        }
        // std::cout << v << std::endl;
        return l;
    }

    friend loader &operator>>(loader &l, uint64_t& v) {
        if (l.is_binary()) {
            l.get().read(reinterpret_cast<char *>(&v), sizeof(uint64_t));
        } else {
            l.get() >> v;
        }
        // std::cout << v << std::endl;
        return l;
    }

    friend loader &operator>>(loader &l, float& v) {
        if (l.is_binary()) {
            l.get().read(reinterpret_cast<char *>(&v), sizeof(float));
        } else {
            l.get() >> v;
        }

        // std::cout << v << std::endl;
        return l;
    }

    friend loader &operator>>(loader &l, bool& v) {
        if (l.is_binary()) {
            l.get().read(reinterpret_cast<char *>(&v), sizeof(bool));
        } else {
            l.get() >> v;
        }
        return l;
    }

    template <typename T>
    friend loader &operator>>(loader &l, std::unique_ptr<T>& v) {
        v.reset(new T());
        l >> *v;
        return l;
    }

    template <typename T>
    friend loader &operator>>(loader &l, p_queue<T>& v) {
        v = p_queue<T>();
        int s;
        l >> s;
        for (int i = 0; i < s; ++i) {
            T tmp;
            l >> tmp;
            v.push(std::move(tmp));
        }
        return l;
    }

    template <typename T1, typename T2>
    friend loader &operator>>(loader &l, std::pair<T1, T2>& v) {
        l >> v.first >> v.second;
        return l;
    }

    friend loader &operator>>(loader &l, std::string& v) {
        if (l.is_binary()) {
            int s;
            l >> s;
            // std::cout << "In loader::string: s = " << s << std::endl;
            if (s > 0) {
                char *tmp = new char[s];
                l.get().read(tmp, s);
                v.assign(tmp, s);
                delete [] tmp;
            } else {
                v = "";
            }
        } else {
            std::string skip;
            std::getline(l.get(), skip, '"');
            std::getline(l.get(), v, '"');
        }

        // std::cout << "\"" << v << "\"" << std::endl;
        return l;
    }

    template <typename T>
    friend loader &operator>>(loader &l, std::vector<T>& v) {
        int s;
        l >> s;
        v.clear();
        for (int i = 0; i < s; ++i) {
            v.emplace_back(T());
            l >> v[i];
        }
        return l;
    }

    template <typename Key, typename T>
    friend loader &operator>>(loader &l, std::map<Key, T>& m) {
        int s;
        l >> s;
        m.clear();
        for (int i = 0; i < s; ++i) {
            Key k;
            T v;
            l >> k >> v;
            m.emplace(std::move(k), std::move(v));
        }
        return l;
    }

    template <typename Key, typename T>
    friend loader &operator>>(loader &l, std::unordered_map<Key, T>& m) {
        // Warning. The order of traversal in unordered_map is not determined. 
        // So you might see different serialization for the same unordered map. 
        int s;
        l >> s;
        m.clear();
        for (int i = 0; i < s; ++i) {
            Key k;
            T v;
            l >> k >> v;
            m.emplace(std::move(k), std::move(v));
        }
        return l;
    }

};

template <typename T, typename... Args>
saver &Save(saver &s, const T &first, const Args&... args) {
    if (! s.is_binary()) s.get() << " ";
    s << first;
    if (! s.is_binary()) s.get() << " ";
    return Save(s, args...);
}

template <typename T>
saver &Save(saver &s, const T &first) {
    if (! s.is_binary()) s.get() << " ";
    s << first;
    if (! s.is_binary()) s.get() << " ";
    return s;
}

template <typename T, typename... Args>
loader &Load(loader &l, T &first, Args&... args) {
    l >> first;
    return Load(l, args...);
}

template <typename T>
loader &Load(loader &l, T &first) {
    l >> first;
    return l;
}

// Get hash code.
inline void _get_hash_code(uint64_t) { }

// From boost.
template <class T>
inline void hash_combine(uint64_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

template <typename T, typename... Args>
void _get_hash_code(uint64_t &seed, const T &first, const Args&... args) {
    hash_combine(seed, first);
    serializer::_get_hash_code(seed, args...);
}

// hash an arbitrary object.
template <typename T>
uint64_t hash_obj(const T &obj) {
    const char *data = reinterpret_cast<const char *>(&obj);
    uint64_t acc = 14695981039346656037U;
    const uint64_t prime = 1099511628211U;
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        acc = (acc ^ data[i]) * prime;
    }
    return acc;
}

}

// Hash functions for vectors.
namespace std
{
    template<typename T>
    struct hash<vector<T>> {
        uint64_t operator()(const vector<T>& s) const {
            uint64_t code = 0;
            for (const auto &v : s) {
                serializer::hash_combine(code, v);
            }
            return code;
        }
    };
}

#define SERIALIZER(TypeName, ...) \
    serializer::saver &Save(serializer::saver &oo) const { \
        serializer::Save(oo, __VA_ARGS__); \
        if (! oo.is_binary()) oo.get() << " "; \
        return oo; \
    } \
    serializer::loader &Load(serializer::loader &ii) { \
        return serializer::Load(ii, __VA_ARGS__); \
    } \
    friend serializer::saver &operator<<(serializer::saver &oo, const TypeName &p) { \
        return p.Save(oo); \
    } \
    friend serializer::loader &operator>>(serializer::loader &ii, TypeName& p) { \
        return p.Load(ii); \
    } \


#define HASH(S, ...) \
    uint64_t GetHashCode() const { \
        uint64_t seed = 0; \
        serializer::_get_hash_code(seed, __VA_ARGS__); \
        return seed; \
    } \


#define STD_HASH(S) \
    namespace std  \
    { \
        template<> struct hash<S> { uint64_t operator()(const S& s) const { return s.GetHashCode(); } }; \
    } \


class no_such_obj_exception {

};

#define SERIALIZER_BASE(BaseTypeName, ...) \
    virtual std::string _signature() const { return #BaseTypeName; } \
    virtual serializer::saver &Save(serializer::saver &oo) const { \
        serializer::Save(oo, __VA_ARGS__); \
        if (! oo.is_binary()) oo.get() << " "; \
        return oo; \
    } \
    virtual serializer::loader &Load(serializer::loader &ii) { \
        return serializer::Load(ii, __VA_ARGS__); \
    } \

#define SERIALIZER_DERIVED(TypeName, BaseTypeName, ...) \
    std::string _signature() const override { return #TypeName; } \
    serializer::saver &Save(serializer::saver &oo) const override { \
        BaseTypeName::Save(oo); \
        serializer::Save(oo, __VA_ARGS__); \
        if (! oo.is_binary()) oo.get() << " "; \
        return oo; \
    } \
    serializer::loader &Load(serializer::loader &ii) override { \
        BaseTypeName::Load(ii); \
        return serializer::Load(ii, __VA_ARGS__); \
    } \

#define SERIALIZER_DERIVED0(TypeName, BaseTypeName) \
    std::string _signature() const override { return #TypeName; } \

#define SERIALIZER_ANCHOR(AnchorTypeName) \
    static std::map<std::string, std::function<AnchorTypeName *()> > _name2obj; \
    friend serializer::saver &operator<<(serializer::saver &oo, const std::unique_ptr<AnchorTypeName> &p) { \
        oo << p->_signature(); \
        if (! oo.is_binary()) oo.get() << " "; \
        return p->Save(oo); \
    } \
    friend serializer::loader &operator>>(serializer::loader &ii, std::unique_ptr<AnchorTypeName> &p) { \
        std::string identifier; \
        ii >> identifier; \
        auto it = AnchorTypeName::_name2obj.find(identifier); \
        if (it != AnchorTypeName::_name2obj.end()) { \
            AnchorTypeName *obj = it->second(); \
            p.reset(obj); \
            p->Load(ii); \
        } else { \
            std::cout << "\"" << identifier << "\" is not valid! File pos = " << ii.get().tellg() << std::endl; \
            throw no_such_obj_exception(); \
        } \
        return ii; \
    } \

#define UNIQUE_PTR_COMPARE(Type) \
    friend bool operator<(const std::unique_ptr<Type>& p1, const std::unique_ptr<Type>& p2) { \
        return *p1 < *p2; \
    } \

#define S_ITEM(TypeName) { #TypeName, []() { return new TypeName(); } }
#define SERIALIZER_ANCHOR_INIT(AnchorTypeName, ...) std::map<std::string, std::function<AnchorTypeName *()> > AnchorTypeName::_name2obj = { S_ITEM(AnchorTypeName), __VA_ARGS__ }

#define SERIALIZER_ANCHOR_FUNC(AnchorTypeName, TypeName) AnchorTypeName::_name2obj[#TypeName] = []() { return new TypeName(); }

#endif
