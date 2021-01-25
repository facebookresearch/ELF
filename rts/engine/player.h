/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "map.h"
#include "cmd.h"
#include "gamedef.h"
#include <queue>

class Unit;

struct Fog {
    // Fog level: 0 no fog, 100 completely invisible.
    int _fog = 100;
    vector<Unit> _prev_seen_units;  // 该点可见单位

    void MakeInvisible() {  _fog = 100; }  // 让该点不可见
    void SetClear() { _fog = 0; _prev_seen_units.clear(); }
    bool CanSeeTerrain() const { return _fog < 50; }
    bool CanSeeUnit() const { return _fog < 30; }

    void SaveUnit(const Unit &u) {
        _prev_seen_units.push_back(u);
    }

    void ResetFog() {
        _fog = 100;
        _prev_seen_units.clear(); 
    }

    const vector<Unit> &seen_units() const { return _prev_seen_units; }

    SERIALIZER(Fog, _fog, _prev_seen_units);
};

// PlayerPrivilege, Normal player only see within the Fog of War.
// KnowAll Player knows everything and can attack objects outside its FOW.
custom_enum(PlayerPrivilege, PV_NORMAL = 0, PV_KNOW_ALL);


typedef  map<UnitId,vector<UnitId> > Targets;

class Player {
private:
    const RTSMap *_map;

    // Player information.
    PlayerId _player_id;
    std::string _name;

    // Type of players, different player could have different privileges.
    PlayerPrivilege _privilege;

    // Used in score computation.
    // How many resources the player have.
    int _resource;

    // Current fog of war. This containers have the same size as the map.
    vector<Fog> _fogs;

    // Heuristic function for path-planning.
    // Loc x Loc -> min distance (in discrete space).
    // If the key is not in _heuristics, then by default it is l2 distance.
    mutable map< pair<Loc, Loc>, float > _heuristics;

    // Cache for path planning. If the cache is too old, it will recompute.
    // Loc == INVALID: cannot pass / passable by a straight line (In this case, we return first_block = -1.
    mutable map< pair<Loc, Loc>, pair<Tick, Loc> > _cache;
   
    //=====Test=======
    // 跟踪目标
    Targets _targets;

    // 奖励
    float reward;  // 现在的分数
    float last_reward; // 上一轮的分数
    //vector<UnitId> _targets;

private:
    struct Item {
        float g;
        float h;
        float cost;
        Loc loc;
        Loc loc_from;
        Item(float g, float h, const Loc &loc, const Loc &loc_from)
            : g(g), h(h), loc(loc), loc_from(loc_from) {
            cost = g + h;
        }

        friend bool operator<(const Item &m1, const Item &m2) {
            if (m1.cost > m2.cost) return true;
            if (m1.cost < m2.cost) return false;
            if (m1.g > m2.g) return true;
            if (m1.g < m2.g) return false;
            if (m1.loc < m2.loc) return true;
            if (m1.loc > m2.loc) return false;
            if (m1.loc_from < m2.loc_from) return true;
            if (m1.loc_from > m2.loc_from) return false;
            return true;
        }

        string PrintInfo(const RTSMap &m) const {
            stringstream ss;
            ss << "cost: " << cost << " loc: (" << m.GetCoord(loc) << ") g: " << g << " h: " << h << " from: (" << m.GetCoord(loc_from) << ")";
            return ss.str();
        }
    };

    Loc _filter_with_fow(const Unit& u) const;

    bool line_passable(UnitId id, const PointF &curr, const PointF &target) const;
    float get_line_dist(const Loc &p1, const Loc &p2) const;

    // Update the heuristic value.
    void update_heuristic(const Loc &p1, const Loc &p2, float value) const;

    // Get the heuristic distance from p1 to p2.
    float get_path_dist_heuristic(const Loc &p1, const Loc &p2) const;

public:
    Player() : _map(nullptr), _player_id(INVALID), _privilege(PV_NORMAL), _resource(0) ,reward(0),last_reward(0){
    }
    Player(const RTSMap& m, const std::string &name, int player_id)
        : _map(&m), _player_id(player_id), _name(name), _privilege(PV_NORMAL), _resource(0),reward(0),last_reward(0) {
        _fogs.assign(_map->GetPlaneSize(), Fog());
    }

    const RTSMap& GetMap() const { return *_map; }
    const RTSMap *ResetMap(const RTSMap *new_map) { auto tmp = _map; _map = new_map; return tmp; }
    PlayerId GetId() const { return _player_id; }
    const std::string &GetName() const { return _name; }
    int GetResource() const { return _resource; }
    

    string Draw() const;
    void ComputeFOW(const map<UnitId, unique_ptr<Unit> > &units);
    bool FilterWithFOW(const Unit& u) const;  // true--单位在视野内  false--单位在视野外

    float GetDistanceSquared(const PointF &p, const Coord &c) const {
        float dx = p.x - c.x;
        float dy = p.y - c.y;
        return dx * dx + dy * dy;
    }

    // 计算奖励
    float GetPlayerReward(){return reward - last_reward;}; //计算奖励
    void UpdateLastReward(){last_reward = reward;}; // 更新上一轮的奖励
    void ChangeReward(float change){reward += change;}; //改变奖励
    float GetLastReward(){return last_reward;};
    float GetReward(){return reward;};

    // It will change _heuristics internally.
    bool PathPlanning(Tick tick, UnitId id, const PointF &curr, const PointF &target, int max_iteration, bool verbose, Coord *first_block, float *est_dist) const;

    void SetPrivilege(PlayerPrivilege new_pv) { _privilege = new_pv; }
    PlayerPrivilege GetPrivilege() const { return _privilege; }

    int ChangeResource(int delta) {
        _resource += delta;
        // cout << "Base resource = " << _resource << endl;
        return _resource;
    }

    string DrawStats() const {
        return make_string("p", _player_id, _resource);
    }

    void ClearCache() { 
        _heuristics.clear(); 
        _cache.clear(); 
        _resource = 0; 
        last_reward =0;
        reward = 0;
        for (auto &fog : _fogs) {
            fog.ResetFog();
        }
        
        // Test
        _targets.clear();
    }

    const Fog &GetFog(Loc loc) const { return _fogs[loc]; }

    string PrintInfo() const;

    string PrintHeuristicsCache() const;

    // 24-30 encoding player id.
    static PlayerId ExtractPlayerId(UnitId id) { return (id >> 24); }
    static UnitId CombinePlayerId(UnitId raw_id, PlayerId player_id) { return (raw_id & 0xffffff) | (player_id << 24); }

    SERIALIZER(Player, _player_id, _name, _privilege, _resource, _fogs, _heuristics, _cache);
    HASH(Player, _player_id, _privilege, _resource);
    
    //跟踪目标的方法
    map<UnitId,vector<UnitId> >& GetTargets(){return _targets;}  // 返回玩家锁定目标
    void AddRadar(UnitId radar_id);     // 增加一个雷达 （锁定目标）
    void RemoveRadar(UnitId radar_id);  // 去除一个雷达  (锁定目标)
    void AddUnit(UnitId radar_id,UnitId target_id); //锁定
    bool RemoveUnit(UnitId target_id);  // 解除锁定
    bool isUnitLocked(UnitId target_id) const;  //查看一个单位是否已经被锁定
    string PrintTargetInfo(UnitId radar_id = -1) ;

};

STD_HASH(Player);

#endif
