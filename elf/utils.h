#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <map>
#include <sstream>
#include <chrono>

namespace elf_utils {

using namespace std;

template <typename K, typename V>
const V &map_get(const unordered_map<K, V> &m, const K& k, const V &def) {
    auto it = m.find(k);
    if (it == m.end()) return def;
    else return it->second;
}

template <typename K, typename V>
V map_get(const unordered_map<K, V> &m, const K& k, V def) {
    auto it = m.find(k);
    if (it == m.end()) return def;
    else return it->second;
}


template <typename K, typename V>
pair<typename unordered_map<K, V>::const_iterator, bool> map_get(const unordered_map<K, V> &m, const K& k) {
    auto it = m.find(k);
    if (it == m.end()) {
        return make_pair(m.end(), false);
    } else {
        return make_pair(it, true);
    }
}

template <typename K, typename V>
pair<typename unordered_map<K, V>::iterator, bool> map_get(unordered_map<K, V> &m, const K& k) {
    auto it = m.find(k);
    if (it == m.end()) {
        return make_pair(m.end(), false);
    } else {
        return make_pair(it, true);
    }
}

template <typename K, typename V>
pair<typename unordered_map<K, V>::iterator, bool> sync_add_entry(unordered_map<K, V> &m, mutex &mut, const K& k, function <V ()> gen) { 
    auto res = map_get(m, k);
    if (res.second) return res;

    // Otherwise allocate
    lock_guard<mutex> lock(mut);

    // We need to do that again to enforce that one node does not get allocated twice. 
    res = map_get(m, k);
    if (res.second) return res;

    // Save it back. This visit doesn't count.
    return m.insert(make_pair(k, gen()));
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
        // cout << "Record: " << item << endl;
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
