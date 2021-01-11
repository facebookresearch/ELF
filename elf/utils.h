/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <map>
#include <sstream>
#include <chrono>

namespace elf_utils {

using namespace std;

inline string print_bool(bool b) { return b ? "True" : "False"; }

inline string trim(string& str) {
    str.erase(0, str.find_first_not_of(' '));       //prefixing spaces
    str.erase(str.find_last_not_of(' ')+1);         //surfixing spaces
    return str;
}

inline vector<string> split(const string &s, char delim) {
    stringstream ss(s);
    string item;
    vector<string> elems;
    while (getline(ss, item, delim)) {
        elems.push_back(move(item));
    }
    return elems;
}

template <typename Map>
const typename Map::mapped_type &map_get(const Map &m, const typename Map::key_type& k, const typename Map::mapped_type &def) {
    auto it = m.find(k);
    if (it == m.end()) return def;
    else return it->second;
}

template <typename Map>
const typename Map::mapped_type &map_inc(Map &m, const typename Map::key_type& k, const typename Map::mapped_type &default_value) {
    auto it = m.find(k);
    if (it == m.end()) {
      auto res = m.insert(make_pair(k, default_value));
      return res.first->second;
    } else {
      it->second ++;
      return it->second;
    }
}

/*
template <typename Map>
typename Map::mapped_type map_get(const Map &m, const typename Map::key_type& k, typename Map::mapped_type def) {
    auto it = m.find(k);
    if (it == m.end()) return def;
    else return it->second;
}
*/

template <typename Map>
pair<typename Map::const_iterator, bool> map_get(const Map &m, const typename Map::key_type& k) {
    auto it = m.find(k);
    if (it == m.end()) {
        return make_pair(m.end(), false);
    } else {
        return make_pair(it, true);
    }
}

template <typename Map>
pair<typename Map::iterator, bool> map_get(Map &m, const typename Map::key_type& k) {
    auto it = m.find(k);
    if (it == m.end()) {
        return make_pair(m.end(), false);
    } else {
        return make_pair(it, true);
    }
}

class MyClock {
private:
    chrono::time_point<chrono::system_clock> _time_start;
    map<string, pair<chrono::duration<double>, int> > _durations;
public:
    MyClock() { }
    void Restart() {
        for (auto it = _durations.begin(); it != _durations.end(); ++it) {
            it->second.first = chrono::duration<double>::zero();
            it->second.second = 0;
        }
        _time_start = chrono::system_clock::now();
    }

    void SetStartPoint() {
        _time_start = chrono::system_clock::now();
    }

    string Summary() const {
        stringstream ss;
        double total_time = 0;
        for (auto it = _durations.begin(); it != _durations.end(); ++it) {
            if (it->second.second > 0) {
                double v = it->second.first.count() * 1000 / it->second.second;
                ss << it->first << ": " << v << "ms. ";
                total_time += v;
            }
        }
        ss << "Total: " << total_time << "ms.";
        return ss.str();
    }

    inline bool Record(const string & item) {
        //cout << "Record: " << item << endl;
        auto it = _durations.find(item);
        if (it == _durations.end()) {
            it = _durations.insert(make_pair(item, make_pair(chrono::duration<double>(0), 0))).first;
        }

        auto time_tmp = chrono::system_clock::now();
        it->second.first += time_tmp - _time_start;
        it->second.second ++;
        _time_start = time_tmp;
        return true;
    }
};

} // namespace elf_utils
