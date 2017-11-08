/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include <cmath>
#include <vector>
#include <unordered_map>
#include <set>
#include <map>
#include <memory>
#include <sstream>
#include <stdarg.h>

#include "custom_enum.h"
#include "serializer.h"

using namespace std;

// All invalid numbers are -1
const int INVALID = -1;
const int MAX_GAME_LENGTH_EXCEEDED = -2;

custom_enum(UnitType, INVALID_UNITTYPE = -1,
    /* Minirts unit types*/
    RESOURCE = 0, WORKER, MELEE_ATTACKER, RANGE_ATTACKER, BARRACKS, BASE, NUM_MINIRTS_UNITTYPE,
    /* Capture the flag unit type */
    FLAG_BASE = 0, FLAG_ATHLETE, FLAG, NUM_FLAG_UNITTYPE,
    /* Tower defense unit type */
    TOWER_BASE = 0, TOWER_ATTACKER, TOWER, NUM_TD_UNITTYPE
    );

custom_enum(UnitAttr, INVALID_UNITATTR = -1, ATTR_NORMAL = 0, ATTR_INVULNERABLE, NUM_UNITATTR);
custom_enum(CDType, INVALID_CDTYPE = -1, CD_MOVE = 0, CD_ATTACK, CD_GATHER, CD_BUILD, NUM_COOLDOWN);
custom_enum(Terrain, INVALID_TERRAIN = -1, NORMAL = 0, IMPASSABLE = 1, FOG = 2);
custom_enum(Level, INVALID_LEVEL = -1, GROUND = 0, AIR, NUM_LEVEL);
custom_enum(BulletState, INVALID_BULLETSTATE = -1, BULLET_READY = 0, BULLET_EXPLODE1, BULLET_EXPLODE2, BULLET_EXPLODE3, BULLET_DONE);

// Map location (as integer)
typedef int Loc;
typedef int UnitId;
typedef int PlayerId;
typedef int Tick;

struct Coord {
    int x, y, z;
    Coord(int x, int y, int z = 0) : x(x), y(y), z(z) { }
    Coord() : x(0), y(0), z(0) { }

    Coord Left() const { Coord c(*this); c.x --; return c; }
    Coord Right() const { Coord c(*this); c.x ++; return c; }
    Coord Up() const { Coord c(*this); c.y --; return c; }
    Coord Down() const { Coord c(*this); c.y ++; return c; }

    friend ostream &operator<<(ostream &oo, const Coord& p) {
        oo << p.x << " " << p.y;
        return oo;
    }

    friend istream &operator>>(istream &ii, Coord& p) {
        ii >> p.x >> p.y;
        p.z = 0;
        return ii;
    }

    SERIALIZER(Coord, x, y, z);
    HASH(Coord, x, y, z);
};

STD_HASH(Coord);

// Seems that this should be larger than \sqrt{2}/2/2 = 0.35355339059
// const float kUnitRadius = 0.36;
const float kUnitRadius = 0.25;

struct PointF {
    float x, y;
    PointF(float x, float y) : x(x), y(y) { }
    PointF() { SetInvalid(); }
    PointF(const Coord& c) :  x(c.x), y(c.y) { }
    Coord ToCoord() const { return Coord((int)(x + 0.5), (int)(y + 0.5)); }

    PointF Left() const { PointF c(*this); c.x -= 1.0; return c; }
    PointF Right() const { PointF c(*this); c.x += 1.0; return c; }
    PointF Up() const { PointF c(*this); c.y -= 1.0; return c; }
    PointF Down() const { PointF c(*this); c.y += 1.0; return c; }

    PointF LT() const { PointF c(*this); c.x -= 1.0; c.y -= 1.0; return c; }
    PointF LB() const { PointF c(*this); c.x -= 1.0; c.y += 1.0; return c; }
    PointF RT() const { PointF c(*this); c.x += 1.0; c.y -= 1.0; return c; }
    PointF RB() const { PointF c(*this); c.x += 1.0; c.y += 1.0; return c; }

    PointF LL() const { PointF c(*this); c.x -= 2.0; return c; }
    PointF RR() const { PointF c(*this); c.x += 2.0; return c; }
    PointF TT() const { PointF c(*this); c.y -= 2.0; return c; }
    PointF BB() const { PointF c(*this); c.y += 2.0; return c; }

    bool IsInvalid() const { return x < -1e17 || y < -1e17; }
    bool IsValid() const { return ! IsInvalid(); }
    void SetInvalid() { x = -1e18; y = -1e18; }

    PointF CCW90() const {
        return PointF(-y, x);
    }
    PointF CW90() const {
        return PointF(y, -x);
    }
    PointF Negate() const {
        return PointF(-y, -x);
    }

    const PointF &Trunc(float mag) {
        float l = std::sqrt(x * x + y * y);
        if (l > mag) {
            x *= mag / l;
            y *= mag / l;
        }
        return *this;
    }

    PointF &operator+=(const PointF &p) {
        x += p.x;
        y += p.y;
        return *this;
    }

    PointF &operator-=(const PointF &p) {
        x -= p.x;
        y -= p.y;
        return *this;
    }

    PointF &operator*=(float s) {
        x *= s;
        y *= s;
        return *this;
    }

    PointF &operator/=(float s) {
        x /= s;
        y /= s;
        return *this;
    }

    friend PointF operator-(const PointF &p1, const PointF &p2) {
        return PointF(p1.x - p2.x, p1.y - p2.y);
    }

    bool IsIn(const PointF &left_top, const PointF& right_down) const {
        return left_top.x <= x && x <= right_down.x && left_top.y <= y && y <= right_down.y;
    }

    static bool Diff(const PointF &p1, const PointF &p2, PointF *d) {
        if (p1.IsInvalid() || p2.IsInvalid()) return false;
        d->x = p1.x - p2.x;
        d->y = p1.y - p2.y;
        return true;
    }

    friend ostream &operator<<(ostream &oo, const PointF& p) {
        oo << p.x << " " << p.y;
        return oo;
    }

    friend istream &operator>>(istream &ii, PointF& p) {
        ii >> p.x >> p.y;
        return ii;
    }

    static float L2Sqr(const PointF& p1, const PointF& p2) {
        float dx = p1.x - p2.x;
        float dy = p1.y - p2.y;
        return dx * dx + dy * dy;
    }

    SERIALIZER(PointF, x, y);
    HASH(PointF, x, y);
};

STD_HASH(PointF);

struct RegionF {
    PointF c;
    float r;
};

// Function that make an arbitrary list into a string, separated by space.
template <typename T>
void _make_string(stringstream &oo, T v) {
    oo << v;
}

template <typename T, typename... Args>
void _make_string(stringstream &oo, T v, Args... args) {
    oo << v << " ";
    _make_string(oo, args...);
}

template <typename... Args>
string make_string(Args... args) {
    stringstream s;
    _make_string(s, args...);
    return s.str();
}

struct Cooldown {
    Tick _last;
    int _cd;
    void Set(int cd_val) { _last = 0; _cd = cd_val; }
    void Start(Tick tick) { _last = tick; }
    bool Passed(Tick tick) const {
        return tick - _last >= _cd;
    }
    string PrintInfo(Tick tick = INVALID) const {
        stringstream ss;
        ss << "_last: " << _last << " cd: " << _cd;
        if (tick != INVALID) ss << "tick: " << tick << ", tick-_last: " << tick - _last;
        return ss.str();
    }

    SERIALIZER(Cooldown, _last, _cd);
    HASH(Cooldown, _cd, _last);
};

STD_HASH(Cooldown);

#endif
