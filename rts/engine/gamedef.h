/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _GAMEDEF_H_
#define _GAMEDEF_H_

#include "cmd.h"
#include "common.h"

// [TODO]: Make it more modular.
// Define unit property.
struct UnitProperty {
    int _hp, _max_hp;

    int _att, _def;
    int _att_r;
    float _speed;

    // Visualization range.
    int _vis_r;

    int _changed_hp;
    UnitId _damage_from;

    // Attributes
    UnitAttr _attr;

    // All CDs.
    vector<Cooldown> _cds;

    // Used for capturing the flag game.
    int _has_flag = 0;

    inline bool IsDead() const { return _hp <= 0; }
    inline Cooldown &CD(CDType t) { return _cds[t]; }
    inline const Cooldown &CD(CDType t) const { return _cds[t]; }
    inline UnitId GetLastDamageFrom() const { return _damage_from; }
    void SetCooldown(int t, Cooldown cd) { _cds[static_cast<CDType>(t)] = cd; }

    // setters for lua
    void SetSpeed(double speed) { _speed = static_cast<float>(speed); }
    void SetAttr(int attr) { _attr = static_cast<UnitAttr>(attr); }

    string Draw(Tick tick) const {
        stringstream ss;
        ss << "Tick: " << tick << " ";
        ss << "H: " << _hp << "/" << _max_hp << " ";
        for (int i = 0; i < NUM_COOLDOWN; ++i) {
            CDType t = (CDType)i;
            int cd_val = CD(t)._cd;
            int diff = std::min(tick - CD(t)._last, cd_val);
            ss << t << " [last=" << CD(t)._last << "][diff=" << diff << "][cd=" << cd_val << "]; ";
        }
        return ss.str();
    }
    inline string PrintInfo() const { return std::to_string(_hp) + "/" + std::to_string(_max_hp); }

    UnitProperty()
        : _hp(0), _max_hp(0), _att(0), _def(0), _att_r(0),
        _speed(0.0), _vis_r(0), _changed_hp(0), _damage_from(INVALID), _attr(ATTR_NORMAL),  _cds(NUM_COOLDOWN) { }

    SERIALIZER(UnitProperty, _hp, _max_hp, _att, _def, _att_r, _speed, _vis_r, _changed_hp, _damage_from, _attr, _cds);
    HASH(UnitProperty, _hp, _max_hp, _att, _def, _att_r, _speed, _vis_r, _changed_hp, _damage_from, _attr, _cds);
};
STD_HASH(UnitProperty);

struct UnitTemplate {
    UnitProperty _property;
    set<CmdType> _allowed_cmds;
    vector<float> _attack_multiplier;
    set<Terrain> _cant_move_over;
    UnitType _build_from;
    vector<BuildSkill> _build_skills;
    int _build_cost;

    UnitTemplate() : _attack_multiplier(NUM_MINIRTS_UNITTYPE, 0.0) {}

    int GetUnitCost() const { return _build_cost; }
    bool CmdAllowed(CmdType cmd) const {
        if (cmd == CMD_DURATIVE_LUA) return true;
        return _allowed_cmds.find(cmd) != _allowed_cmds.end();
    }

    float GetAttackMultiplier(UnitType unit_type) const { return _attack_multiplier[unit_type]; }
    bool CanMoveOver(Terrain terrain) const { return _cant_move_over.find(terrain) == _cant_move_over.end(); }
    const vector<BuildSkill>& GetBuildSkills() const { return _build_skills; }

    UnitType GetUnitTypeFromHotKey(char hotkey) const {
        for (const auto& skill : _build_skills) {
            if (hotkey == skill.GetHotKey()[0]) {
                return skill.GetUnitType();
            }
        }
        return INVALID_UNITTYPE;
    }

    void AddAllowedCmd(int cmd) { _allowed_cmds.insert(static_cast<CmdType>(cmd)); }
    void SetAttackMultiplier(int unit_type, double mult) { _attack_multiplier[unit_type] = static_cast<float>(mult); }
    void AddCantMoveOver(int terrain) { _cant_move_over.insert(static_cast<Terrain>(terrain)); }
    void SetBuildFrom(int unit_type) { _build_from = (UnitType)unit_type; }
    void AddBuildSkill(BuildSkill skill) { _build_skills.push_back(std::move(skill)); }

    void SetProperty(UnitProperty prop) { _property = prop; }

};

UnitTemplate _C(int cost, int hp, int defense, float speed, int att, int att_r, int vis_r,
        const vector<int> &cds, const vector<CmdType> &l, UnitAttr attr = ATTR_NORMAL);

class GameEnv;
class RTSMap;
struct RTSGameOptions;
class RuleActor;

// GameDef class. Have different implementations for different games.
class GameDef {
public:
    std::vector<UnitTemplate> _units;

    // Initialize everything.
    void Init();

    static void GlobalInit();

    // Get the number of unit type in this game
    static int GetNumUnitType();

    // Get the number of game action in this game
    static int GetNumAction();

    static bool IsUnitTypeBuilding(UnitType t);
    bool HasBase() const;

    // Check if the unit can be added at current location p.
    bool CheckAddUnit(RTSMap* _map, UnitType type, const PointF& p) const;

    const UnitTemplate &unit(UnitType t) const {
        if (t < 0 || t >= (int)_units.size()) {
            cout << "UnitType " << t << " is not found!" << endl;
            throw std::range_error("Unit type is not found!");
        }
        return _units[t];
    }

    // Get game initialization commands.
    vector<pair<CmdBPtr, int> > GetInitCmds(const RTSGameOptions& options) const;

    // Check winner for the game.
    PlayerId CheckWinner(const GameEnv& env, bool exceeds_max_tick = false) const;

    // Get implementation for different befaviors on dead units.
    void CmdOnDeadUnitImpl(GameEnv* env, CmdReceiver* receiver, UnitId _id, UnitId _target) const;
};
#endif
