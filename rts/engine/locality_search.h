/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#ifndef _LOCALITY_SEARCH_H_
#define _LOCALITY_SEARCH_H_

#include <limits>
#include <set>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <utility>

#include <iostream>

#include "common.h"

struct LineResult {
    bool passable;
    float dist;
    float t;
};

struct LineCoeff {
    // dist = |A * x + B * y + C|
    float _A, _B, _C;
    // t = t0 + tx * x + ty * y
    float _t0, _tx, _ty;
    //
    float _gy1, _gy0, _gx1, _gx0;

    PointF _p1, _p2;
    float _line_r, _exclusion_r;

    LineCoeff(const PointF &p1, const PointF &p2, float line_r, float exclusion_r)
      : _p1(p1), _p2(p2), _line_r(line_r), _exclusion_r(exclusion_r) {
        const float det = p2.x * p1.y - p1.x * p2.y;
        const float eps = std::numeric_limits<float>::epsilon();
        const float dx = p1.x - p2.x;
        const float dy = p1.y - p2.y;
        const float dist_sqr = dx * dx + dy * dy;

        if (std::abs(det) < eps) {
            _C = 0;
            _A = - p2.y;
            _B = p1.x;
            float norm = sqrt(_A * _A + _B * _B) + eps;
            _A /= norm;
            _B /= norm;
        } else {
            const float dist = sqrt(dist_sqr);
            _C = det / dist;
            _B = dx / dist;
            _A = - dy / dist;
        }
        _tx = dx / dist_sqr;
        _ty = dy / dist_sqr;
        _t0 = - (p2.x * dx + p2.y * dy) / dist_sqr;

        // Compute
        if (std::abs(_A) >= eps) {
            _gy1 = -_B/_A;
            _gy0 = -_C/_A;
        } else {
            _gy1 = std::numeric_limits<float>::signaling_NaN();
            _gy0 = std::numeric_limits<float>::signaling_NaN();
        }
        if (std::abs(_B) >= eps) {
            _gx1 = -_A/_B;
            _gx0 = -_C/_B;
        } else {
            _gx1 = std::numeric_limits<float>::signaling_NaN();
            _gx0 = std::numeric_limits<float>::signaling_NaN();
        }
    }

    // For each point, compute the closest distance with analytic formula.
    void Distance(const PointF &p, LineResult *result) const {
        // Assuming A^2 + B^2 + C^2 = 1.
        // Distance = |A * p.x + B * p.y + C|
        result->dist = std::abs(_A * p.x + _B * p.y + _C);
        result->t = _t0 + _tx * p.x + _ty * p.y;
    }
    float GetYFromX(float x) const { return _gx1 * x + _gx0; }
    float GetXFromY(float y) const { return _gy1 * y + _gy0; }

    bool IsPassable(const PointF &p, float radius, LineResult *result) const {
        if (IsInExclusion(p)) {
            // cout << p << " excluded " << endl;
            result->passable = true;
            return true;
        }
        Distance(p, result);
        // cout << p << " dist = " << result->dist << " t = " << result->t << endl;
        result->passable = (result->t < 0 || result->t > 1 || result->dist > _line_r + radius);
        return result->passable;
    }

    bool IsInExclusion(const PointF &p) const {
        return PointF::L2Sqr(p, _p1) <= _exclusion_r * _exclusion_r || PointF::L2Sqr(p, _p2) <= _exclusion_r * _exclusion_r;
    }
};

template <typename T>
class LocalitySearch {
private:
    PointF _pmin;
    PointF _pmax;
    float _margin;
    int _n;
    int _m;

    using Loc = std::pair<PointF, float>;
    using KeysToLocs = std::unordered_map<T, Loc>;
    // If memory usage is an issue, we can store iterator in the grid instead
    KeysToLocs _keys2locs;
    KeysToLocs _irreg_keys2locs;
    std::vector<std::vector<KeysToLocs>> _grid;

    int GetXBucket(float x) const {
        return static_cast<int>((x - _pmin.x) / _margin);
    }

    int GetXBucket(const PointF& p) const {
        return GetXBucket(p.x);
    }

    int GetYBucket(float y) const {
        return static_cast<int>((y - _pmin.y) / _margin);
    }

    int GetYBucket(const PointF& p) const {
        return GetYBucket(p.y);
    }

    bool IsRegular(const PointF& p, const float radius) const {
        return 2 * radius < _margin + std::numeric_limits<float>::epsilon()
            && p.IsIn(_pmin, _pmax);
    }

    static bool CheckCollision(const Loc& loc1, const Loc& loc2) {
      const float dist_sqr = PointF::L2Sqr(loc1.first, loc2.first);
      const float sum_dist = loc1.second + loc2.second;
      return dist_sqr < sum_dist * sum_dist;
    }

    bool _line_passable(const LineCoeff &c, int x_ind, int y_ind, T* id, LineResult* result) const {
        if (x_ind < 0 || x_ind >= _m || y_ind < 0 || y_ind >= _n) return true;
        for (const auto& item : _grid[x_ind][y_ind]) {
            if (! c.IsPassable(item.second.first, item.second.second, result)) {
                if (id) *id = item.first;
                return false;
            }
        }
        return true;
    }

    bool _line_irregular_passable(const LineCoeff &c, T* id, LineResult *result) const {
        for (const auto& item : _irreg_keys2locs) {
            // cout << "Check irregular " << item.second.first << " radius = " << item.second.second << endl;
            if (! c.IsPassable(item.second.first, item.second.second, result)) {
                if (id) *id = item.first;
                return false;
            }
        }
        return true;
    }

public:
    LocalitySearch() {};

    LocalitySearch(
        const PointF& pmin,
        const PointF& pmax,
        const float max_radius = kUnitRadius)
            : _pmin(pmin), _pmax(pmax), _margin(2 * max_radius) {
        _n = static_cast<int>((_pmax.x - _pmin.x + _margin) / _margin);
        _m = static_cast<int>((_pmax.y - _pmin.y + _margin) / _margin);
        _grid.resize(_n, std::vector<KeysToLocs>(_m));
    }

    // Add location and key
    void Add(const T& key, const PointF& p, const float radius) {
        const auto loc = Loc(p, radius);
        _keys2locs.emplace(key, loc);
        if (IsRegular(p, radius)) {
            int x_i = GetXBucket(p);
            int y_i = GetYBucket(p);
            // cout << "Add " << p << " to (" << x_i << ", " << y_i << ")" << endl;
            _grid[x_i][y_i].emplace(key, loc);
        } else {
            // cout << "Add " << p << " to irregular spot" << " # size = " << _irreg_keys2locs.size() << endl;
            _irreg_keys2locs.emplace(key, loc);
        }
    }

    bool Exists(const T& key) const {
        return _keys2locs.find(key) != _keys2locs.end();
    }

    bool IsEmpty(const PointF& p, const float radius,
        const T& key_exclude = INVALID) const {
        const auto loc = Loc(p, radius);
        if (!IsRegular(p, radius)) {
            for (const auto& item : _irreg_keys2locs) {
                if (item.first == key_exclude) continue;
                if (CheckCollision(loc, item.second)) {
                    return false;
                }
            }
            return true;
        }
        const int bx = GetXBucket(p);
        const int by = GetYBucket(p);
        // Explore 8 adjacent blocks as well
        for (int dx = -1; dx <= 1; ++dx) {
            if (bx + dx < 0 || bx + dx >= _n) continue;
            for (int dy = -1; dy <= 1; ++dy) {
                if (by + dy < 0 || by + dy >= _m) continue;
                for (const auto& item : _grid[bx + dx][by + dy]) {
                    if (item.first == key_exclude) continue;
                    if (CheckCollision(loc, item.second)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    // Check whether a line of a given radius can pass though all the points.
    // Note that we exclude the region around p1 and p2.
    bool LinePassable(const PointF &p1, const PointF &p2, float line_radius, float exclusion_radius, T* id, LineResult *result) const {
        LineCoeff c(p1, p2, line_radius, exclusion_radius);
        bool line_passable = true;
        const PointF *p_min = &p1;
        const PointF *p_max = &p2;

        if (! _line_irregular_passable(c, id, result)) return false;

        // Compute the bucket we want to check on.
        // Pick the coordinates with smaller magnitude.
        if (abs(c._A) > abs(c._B)) {
            // We loop over y.
            if (p_min->y > p_max->y) std::swap(p_min, p_max);

            // Go through the bucket and check
            int idx_min = GetYBucket(p_min->y);
            int idx_max = GetYBucket(p_max->y);
            // cout << "Loop over y: idx_min = " << idx_min << ", idx_max = " << idx_max << endl;

            for (int y_i = idx_min; y_i <= idx_max && line_passable; ++y_i) {
                // Get X bucket.
                float y = p_min->y + (y_i - idx_min) * _margin;
                float x = c.GetXFromY(y);
                int x_i = GetXBucket(x);
                // cout << "(x, y) = (" << x << ", " << y << ")" << " (x_i, y_i) = (" << x_i << ", " << y_i << ")" << endl;
                // Check x_i - 1, x_i and x_i + 1.
                line_passable = _line_passable(c, x_i, y_i, id, result) \
                                && _line_passable(c, x_i - 1, y_i, id, result) \
                                && _line_passable(c, x_i + 1, y_i, id, result);
            }
        } else {
            if (p_min->x > p_max->x) std::swap(p_min, p_max);

            // Go through the bucket and check
            int idx_min = GetXBucket(p_min->x);
            int idx_max = GetXBucket(p_max->x);

            // cout << "Loop over x: idx_min = " << idx_min << ", idx_max = " << idx_max << endl;

            for (int x_i = idx_min; x_i <= idx_max && line_passable; ++x_i) {
                // Get Y bucket.
                float x = p_min->x + (x_i - idx_min) * _margin;
                float y = c.GetYFromX(x);
                int y_i = GetYBucket(y);
                // cout << "(x, y) = (" << x << ", " << y << ")" << " (x_i, y_i) = (" << x_i << ", " << y_i << ")" << endl;
                // Check y_i - 1, y_i and y_i + 1.
                line_passable = _line_passable(c, x_i, y_i, id, result) \
                                && _line_passable(c, x_i, y_i - 1, id, result) \
                                && _line_passable(c, x_i, y_i + 1, id, result);
            }
        }
        return line_passable;
    }

    // Remove the entry.
    void Remove(const T& key) {
        const auto it = _keys2locs.find(key);
        if (it != _keys2locs.end()) {
            const auto& p = it->second.first;
            if (IsRegular(p, it->second.second)) {
                _grid[GetXBucket(p)][GetYBucket(p)].erase(key);
            } else {
                _irreg_keys2locs.erase(key);
            }
            _keys2locs.erase(it);
        }
    }

    // Retrieval.
    // Find the object within its radius.
    const T* Loc2Key(const PointF& p, float* const min_dist_sqr) const {
        const T* res = nullptr;
        float min_dist = std::numeric_limits<float>::max();
        for (const auto& item : _keys2locs) {
            const auto& q = item.second.first;
            const float r = item.second.second;
            const float dist_sqr = PointF::L2Sqr(p, q);
            if (dist_sqr < r * r && dist_sqr < min_dist) {
                res = &item.first;
                min_dist = dist_sqr;
            }
        }
        *min_dist_sqr = min_dist;
        return res;
    }

    const PointF* Key2Loc(const T& key) const {
        const auto it = _keys2locs.find(key);
        return it == _keys2locs.end() ? nullptr : &it->second->first;
    }

    std::set<T> KeysInRegion(
        const PointF& left_top,
        const PointF& right_bottom) const {
        std::set<T> res;
        for (const auto& item : _keys2locs) {
            const auto& p = item.second.first;
            if (p.IsIn(left_top, right_bottom)) {
                res.insert(item.first);
            }
        }
        return res;
    }

    void Clear() {
        _keys2locs.clear();
        _irreg_keys2locs.clear();
        for (auto& row : _grid) {
            for (auto& item : row) {
                item.clear();
            }
        }
    }

    std::string PrintDebugInfo() const {
        std::stringstream ss;
        ss << "Locality table: " << endl;
        for (const auto& item : _keys2locs) {
            ss << "Id " << item.first << " -> " << item.second.first << ", "
                << item.second.second << std::endl;
        }
        return ss.str();
    }

    SERIALIZER(LocalitySearch,
        _pmin, _pmax, _margin, _keys2locs, _irreg_keys2locs, _grid);
};

#endif
