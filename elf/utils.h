#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace elf_utils {

using namespace std;

template <typename K, typename V>
const V &map_get(const unordered_map<K, V> &m, const K& k, const V &def) {
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

} // namespace elf_utils
