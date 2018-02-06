/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "player.h"
#include "unit.h"

#include <set>

template <typename T>
static bool GetValue(const map< pair<Loc, Loc>, T > &m, const Loc &p1, const Loc &p2, T *value) {
    auto it = m.find(make_pair(p1, p2));
    if (it != m.end()) {
        *value = it->second;
        return true;
    }
    return false;
}

template <typename T>
static void UpdateValue(const Loc &p1, const Loc &p2, const T& value, map< pair<Loc, Loc>, T > *m) {
    pair<Loc, Loc> new_key(p1, p2);

    auto it = m->find(new_key);
    if (it != m->end()) {
        it->second = value;
    } else {
        m->emplace(make_pair(std::move(new_key), value));
    }
}

///////////// Player ///////////////////
string Player::Draw() const {
    stringstream ss;
    ss << "m " << _map->GetXSize() << " " << _map->GetYSize() << " " << endl;
    for (int y = 0; y < _map->GetYSize(); ++y) {
        for (int x = 0; x < _map->GetXSize(); ++x) {
            // Draw the map (only level 0)
            Loc loc = _map->GetLoc(Coord(x, y, 0));
            if ( _fogs[loc].CanSeeTerrain() ) {
                ss << (*_map)(loc).type << " ";
            } else {
                ss << "# ";
            }
        }
        ss << endl;
    }
    return ss.str();
}

void Player::CreateFog() {
    _fogs.assign(_map->GetPlaneSize(), Fog());
}

void Player::ComputeFOW(const Units &units) {
    // Compute the player's fog of war.
    // Two approaches, loop on the map (or in the future, viewport [TODO]),
    // or loop on the units.
    // [TODO]: We could do better with LocalitySearch.
    // Clear fogs.
    for (Fog &f : _fogs) {
        f.MakeInvisible();
    }


    // First pass, get the fog region.
    set<Loc> clear_regions;
    for (auto it = units.begin(); it != units.end(); ++it) {
        const Unit *u = it->second.get();
        if (ExtractPlayerId(u->GetId()) == _player_id) {
            const int vis_r = u->GetProperty()._vis_r;
            Loc l = _map->GetLoc(u->GetPointF().ToCoord());
            for (const Loc &loc : _map->GetSight(l, vis_r)) {
                clear_regions.insert(loc);
            }
        }
    }

    // Clear these fog unit.
    for (const Loc &l : clear_regions) {
        _fogs[l].SetClear();
    }

    // Second pass, remember the units that was in FoW
    for (auto it = units.begin(); it != units.end(); ++it) {
        const Unit *u = it->second.get();
        if (ExtractPlayerId(u->GetId()) != _player_id) {
            Loc l = _filter_with_fow(*u);
            // Add the unit info to the loc.
            if (l != -1) _fogs[l].SaveUnit(*u);
        }
    }
}

Loc Player::_filter_with_fow(const Unit& u) const {
    if (! _map->IsIn(u.GetPointF())) return -1;
    // [TODO]: Could we do better?
    Loc l = _map->GetLoc(u.GetPointF().ToCoord());
    return (_fogs[l].CanSeeUnit() ? l : -1);
}

bool Player::FilterWithFOW(const Unit& u) const {
    return _filter_with_fow(u) != -1;
}

string Player::PrintInfo() const {
    stringstream ss;
    ss << "Map ptr = " << _map << endl;
    ss << "Player id = " << _player_id << endl;
    ss << "Resource = " << _resource << endl;
    ss << "Fog[" << _fogs.size() << "] = ";
    for (size_t i = 0; i < _fogs.size(); ++i) ss << _fogs[i]._fog << " ";
    ss << endl;

    return ss.str();
}

void Player::update_heuristic(const Loc &p1, const Loc &p2, float value) const {
    float min_value = get_line_dist(p1, p2);
    if (value < min_value) value = min_value;
    UpdateValue(p1, p2, value, &_heuristics);
}

float Player::get_line_dist(const Loc &p1, const Loc &p2) const {
    Coord c1 = _map->GetCoord(p1);
    Coord c2 = _map->GetCoord(p2);
    const int dx = c1.x - c2.x;
    const int dy = c1.y - c2.y;
    return sqrt(static_cast<float>(dx * dx + dy * dy));
}

float Player::get_path_dist_heuristic(const Loc &p1, const Loc &p2) const {
    float dist;
    if (! GetValue(_heuristics, p1, p2, &dist)) {
        dist = get_line_dist(p1, p2);
    }
    return dist;
}

bool Player::line_passable(UnitId id, const PointF &s, const PointF &t) const {
    const RTSMap &m = *_map;

    float dist = sqrt(PointF::L2Sqr(s, t));
    const int n = static_cast<int>(dist * 10 + 0.5);

    PointF v;
    PointF::Diff(t, s, &v);
    v /= dist;

    Loc ls = m.GetLoc(s.ToCoord());
    Loc lt = m.GetLoc(t.ToCoord());

    const float step = dist / n;

    Loc last_lx = INVALID;

    for (int i = 1; i < n; ++i) {
        // Check discrete points {1 / n, ..., (n - 1) / n}
        PointF x(s.x + v.x * step * i, s.y + v.y * step * i);
        // cout << "LinePassable[" << id << "]: Checking " << x << ". (s, t) = (" << s << ", " << t << ")" << endl;
        Coord cx = x.ToCoord();
        Loc lx = m.GetLoc(cx);
        if (lx == last_lx) continue;
        last_lx = lx;

        if (lx != ls && lx != lt) {
            if (! m.CanPass(x, id)) {
                // cout << "(" << s << ") -> (" << t << ") line not passable due to (" << x << ")" << endl;
                return false;
            }
        }
    }

    // cout << "(" << s << ") -> (" << t << ") line passable!" << endl;
    return true;
}

/*
bool Player::line_passable(UnitId id, const PointF &s, const PointF &t) const {
    return _map->IsLinePassable(s, t);
}
*/

bool Player::PathPlanning(Tick tick, UnitId id, const PointF &s, const PointF &t, int max_iteration, bool verbose, Coord *first_block, float *dist) const {
    const RTSMap &m = *_map;

    Coord cs = s.ToCoord();
    Coord ct = t.ToCoord();

    Loc ls = m.GetLoc(cs);
    Loc lt = m.GetLoc(ct);

    if (verbose) {
        cout << "[PathPlanning] Tick: " << tick << ", id: " << id << " Start: (" << s << ")" << " Target: (" << t << ") " << " ls = " << ls << ", lt = " << lt << endl;
    }

    first_block->x = first_block->y = -1;
    // Initialize to be the maximal distance.
    *dist = 1e38;

    // Check cache. If the recomputation is fresh, just use it.
    auto it_cache = _cache.find(make_pair(ls, lt));
    if (it_cache != _cache.end()) {
        if (tick - it_cache->second.first < 10) {
            Loc loc = it_cache->second.second;
            if (verbose) cout << "Cache hit! Tick: " << tick << " cache timestamp: " << it_cache->second.first << " Loc: " << loc << endl;
            if (loc != INVALID) {
                *first_block = m.GetCoord(loc);
            }
            return true;
        } else {
            if (verbose) cout << "Cache out of date! Tick: " << tick << " cache timestamp: " << it_cache->second.first << endl;
            _cache.erase(it_cache);
        }
    }

    // Check if the two points are passable by a straight line. (Most common case).
    if (line_passable(id, s, t)) {
        _cache[make_pair(ls, lt)] = make_pair(tick, INVALID);
        return true;
    }

    // 8 neighbors.
    // const int dx[] = { 1, 0, -1, 0, 1, 1, -1, -1 };
    // const int dy[] = { 0, 1, 0, -1, 1, -1, 1, -1 };
    // const float dists[] = { 1.0, 1.0, 1.0, 1.0, kSqrt2, kSqrt2, kSqrt2, kSqrt2 };

    const int dx[] = { 1, 0, -1, 0 };
    const int dy[] = { 0, 1, 0, -1 };
    const float dists[] = { 1.0, 1.0, 1.0, 1.0 };

    // All "from" information.
    // Loc -> Loc_from, dist_so_far.
    map<Loc, pair<Loc, float> > c_from;
    vector<Loc> c_popped;

    // If s and t is not passable by a straight line.
    priority_queue<Item> q;

    float h0 = get_path_dist_heuristic(ls, lt);
    q.emplace(Item(0.0, h0, ls, INVALID));
    c_from.emplace(make_pair(ls, make_pair(INVALID, 0.0)));

    if (verbose) {
        cout << "Initial h0 = " << h0 << endl;
    }

    int iter = 1;
    Loc l = INVALID;
    bool found = false;

    while (!q.empty()) {
        Item v = q.top();
        // cout << "Poped: " << v.PrintInfo(m) << endl;

        // Save the current node to "from" information.
        // Only the first occurrence matters (since given the same state, smallest g will come first).
        c_popped.push_back(v.loc);
        q.pop();

        // Find the target, stop.
        if (v.loc == lt || iter == max_iteration) {
            if (verbose) {
                cout << "[PathFinding] Tick: " << tick << "  Find the target! cost = " << v.cost << endl;
            }
            *dist = v.cost;
            found = true;
            l = v.loc;
            break;
        }

        Coord c_curr = m.GetCoord(v.loc);
        // Expand to 8 neighbor.
        for (size_t i = 0; i < sizeof(dx) / sizeof(int); ++i) {
            Coord next(c_curr.x + dx[i], c_curr.y + dy[i]);
            Loc l_next = m.GetLoc(next);

            // If we already push that before, skip.
            if (c_from.find(l_next) != c_from.end()) continue;

            // if we met with impassable location and has not reached the target (lt), skip.
            if (l_next != lt) {
               if (GetDistanceSquared(s, next) >= 4 && ! m.CanPass(next, id, false)) continue;
               if (GetDistanceSquared(s, next) < 4 && ! m.CanPass(next, id)) continue;
           }

            float h = get_path_dist_heuristic(l_next, lt);
            float next_dist = v.g + dists[i];

            if (verbose) {
                cout << "push: l_next = " << l_next << ", next_dist = " << next_dist << ", h = " << h << ", parent_loc = " << v.loc << endl;
            }

            q.emplace(Item(next_dist, h, l_next, v.loc));
            c_from.emplace(make_pair(l_next, make_pair(v.loc, next_dist)));
        }
        iter ++;
    }

    if (verbose) {
        cout << "Total iter = " << iter << " optimal dist = " << *dist << endl;
    }

    if (! found) {
        // Not found.
        return false;
    }

    // Then do a backtrace to get the path.
    vector<Loc> traj;
    // traj[0] is the last part of the trajectory, depending on max_iteration,
    // it might end in the target location, or reach some intermediate location, which is the most promising.
    // traj[-1] is the starting point.
    while (l != INVALID) {
        // cout << "Loc: " << m.GetCoord(l) << endl;
        auto it = c_from.find(l);
        if (it == c_from.end()) {
            cout << "Path-finding error!" << endl;
            return false;
        }

        traj.push_back(l);
        update_heuristic(l, lt, *dist - it->second.second);
        l = it->second.first;
    }

    // Delete trajectory from the map.
    // cout << " Traj: ";
    for (const Loc &l : traj) {
        // cout << "(" << m.GetCoord(l) << ") ";
        c_from.erase(l);
    }
    // cout << endl;

    // For all visited node other than the true solution,
    // their heuristic will be set to be opt + eps - g
    for (Loc l : c_popped) {
        auto it = c_from.find(l);
        if (it != c_from.end()) {
            update_heuristic(l, lt, (*dist + 1e-5f) - it->second.second);
        }
    }

    // Compute the first waypoint from the starting.
    // Starting from the end of path and check.
    for (size_t i = 0; i < traj.size(); i++) {
        Coord waypoint = m.GetCoord(traj[i]);
        if (line_passable(id, s, PointF(waypoint.x, waypoint.y))) {
            *first_block = waypoint;
            _cache[make_pair(ls, lt)] = make_pair(tick, traj[i]);
            return true;
        }
    }
    // cout << "PathPlanning. No valid path, leave to local planning" << endl;
    _cache[make_pair(ls, lt)] = make_pair(tick, INVALID);

    return false;
}

string Player::PrintHeuristicsCache() const {
    stringstream ss;
    ss << "Heuristics: " << endl;
    for (auto it = _heuristics.begin(); it != _heuristics.end(); ++it) {
        ss << "[" << it->first.first << ", " << it->first.second << "]: " << it->second << endl;
    }

    ss << "Cache: " << endl;
    for (auto it = _cache.begin(); it != _cache.end(); ++it) {
        ss << "[" << it->first.first << ", " << it->first.second << "]: T " << it->second.first << ": " << it->second.second << endl;
    }
    return ss.str();
}
