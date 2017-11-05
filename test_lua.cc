#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include "lua.hpp"
#include "selene.h"

using namespace std;

struct PointF {
    float x;
    float y;
    PointF(float x = 0, float y = 0) : x(x), y(y) { }
    string info() const {
        stringstream ss;
        ss << "(" << x << ", " << y << ")";
        return ss.str();
    }
};

struct Bar {
    int x;
    Bar(int x_, int x2_) : x(x_) { x += x2_;  }
    int AddThis(int y) const { return x + y; }
    double SubThis(int y) const { return static_cast<double>(x - y); }
    PointF loc() const { return PointF(x, x); }
    double abs() const { return std::abs(x); }
};

struct Unit {
    int hp;
    int att;
    int def;
    Unit(int hp, int att, int def) : hp(hp), att(att), def(def) { }
    Unit() : hp(0), att(0), def(0) { }
    string info() const {
        stringstream ss;
        ss << "hp: " << hp << ", att: " << att << ", def: " << def;
        return ss.str();
    }
};

struct LuaEnv {
public:
    LuaEnv() {
        _units.insert(make_pair(0, Unit(100, 2, 1)));
        _units.insert(make_pair(1, Unit(200, 4, 2)));
        _units.insert(make_pair(2, Unit(800, 12, 2)));
    }
    const Unit &GetUnit(int id) const {
        auto it = _units.find(id);
        if (it != _units.end()) {
            return it->second;
        } else {
            return _empty_unit;
        }
    } 
private:
    map<int, Unit> _units;
    Unit _empty_unit;
};

int main() {
	sel::State state{true};
    LuaEnv env;

    state["global_float"] = []() { return 1.2; };

    state["Unit"].SetClass<Unit, int, int, int>("info", &Unit::info);
    state["env"].SetObj(env, "unit", &LuaEnv::GetUnit);

    state["PointF"].SetClass<PointF>("info", &PointF::info);
	state["Bar"].SetClass<Bar, int, int>(
            "add_this", &Bar::AddThis, 
            "sub_this", &Bar::SubThis, 
            "abs", &Bar::abs,
            "loc", &Bar::loc
    );

	state.Load("test_lua.lua");

    return 0;
}
