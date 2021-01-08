/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#ifndef _GAME_DEV_H_
#define _GAME_DEV_H_

#include "cmd_receiver.h"
#include "unit.h"
#include "bullet.h"
#include "map.h"
#include "player.h"
#include <random>

class GameEnv {
private:
    // Game definitions.
    GameDef _gamedef;

    // Game counter.
    int _game_counter;

    // Next unit_id, initialized to be 0
    UnitId _next_unit_id;

    // Unit hash tables.
    Units _units;

    // Bullet tables.
    Bullets _bullets;

    // The game map.
    unique_ptr<RTSMap> _map;

    // Players
    vector<Player> _players;

    // Random number generator
    std::mt19937 _rng;

    // Who won the game?
    PlayerId _winner_id;

    // Whether the game is terminated.
    // This happens if the time tick exceeds max_tick, or there is anything wrong.
    bool _terminated;

public:
    GameEnv();

    void Visualize() const;

    // Remove all players.
    void ClearAllPlayers();

    // Reset RTS game to starting position
    void Reset();

    // Add and remove players.
    void AddPlayer(const std::string &name, PlayerPrivilege pv);
    void RemovePlayer();

    int GetNumOfPlayers() const { return _players.size(); }
    int GetGameCounter() const { return _game_counter; }

    // Set seed from the random generator.
    void SetSeed(unsigned long seed) { _rng.seed(seed); }

    // Return a random integer from 0 to r - 1
    std::function<uint16_t(int)> GetRandomFunc() {
        return [&](int r) -> uint16_t { return _rng() % r; };
    }
    const RTSMap &GetMap() const { return *_map; }
    RTSMap &GetMap() { return *_map; }

    // Generate a RTSMap given number of obstacles.
    bool GenerateMap(int num_obstacles, int init_resource);
    bool GenerateImpassable(int num_obstacles);

    // Generate a maze used by Tower Defense.
    bool GenerateTDMaze();

    const Units& GetUnits() const { return _units; }
    Units& GetUnits() { return _units; }

    // Initialize different units for this game.
    void InitGameDef() {
        //std::cout<<"----------InitGameDef-------"<<std::endl;
        _gamedef.Init();
    }
    const GameDef &GetGameDef() const { return _gamedef; }

    // Get a unit from its Id.
    const Unit *GetUnit(UnitId id) const {
        const Unit *target = nullptr;
        if (id != INVALID) {
            auto it = _units.find(id);
            if (it != _units.end()) target = it->second.get();
        }
        return target;
    }
    Unit *GetUnit(UnitId id) {
        Unit *target = nullptr;
        if (id != INVALID) {
            auto it = _units.find(id);
            if (it != _units.end()) target = it->second.get();
        }
        return target;
    }

    // Find the closest base.
    UnitId FindClosestBase(PlayerId player_id) const;

    // Find empty place near a place, used by creating units.
    bool FindEmptyPlaceNearby(const PointF &p, int l1_radius, PointF *res_p) const;

    // Find empty place near a place, used by creating buildings.
    bool FindBuildPlaceNearby(const PointF &p, int l1_radius, PointF *res_p) const;

    // Find closest place to a group with a certain distance, used by hit and run.
    bool FindClosestPlaceWithDistance(const PointF &p, int l1_radius,
            const vector<const Unit *>& units, PointF *res_p) const;

    const Player &GetPlayer(PlayerId player_id) const {
      assert(player_id >= 0 && player_id < (int)_players.size());
      return _players[player_id];
    }
    Player &GetPlayer(PlayerId player_id) {
      assert(player_id >= 0 && player_id < (int)_players.size());
      return _players[player_id];
    }

    // Add and remove units.
    bool AddUnit(Tick tick, UnitType type, const PointF &p, PlayerId player_id,UnitId &u_id );
    bool RemoveUnit(const UnitId &id);

    void AddBullet(const Bullet &b) { _bullets.push_back(b); }

    // Check if one player's base has been destroyed.
    PlayerId CheckBase(UnitType base_type) const;
     // 修改判断胜负的方法
    // 判断谁赢并返回玩家ID
    PlayerId CheckWinner(UnitType base_type) const;



    // Getter and setter for winner_id, termination.
    void SetWinnerId(PlayerId winner_id) { _winner_id = winner_id; }
    PlayerId GetWinnerId() const { return _winner_id; }
    void SetTermination() { _terminated = true; }
    bool GetTermination() const { return _terminated; }

    // Compute bullets cmds.
    void Forward(CmdReceiver *receiver);
    void ComputeFOW();

    // Some debug code.
    int GetPrevSeenCount(PlayerId) const;

    // Fill in metadata to a save_class
    template <typename save_class, typename T>
    void FillHeader(const CmdReceiver& receiver, T *game) const {
        save_class::SetTick(receiver.GetTick(), game);
        save_class::SetWinner(_winner_id, game);
        save_class::SetTermination(_terminated, game);
    }

    // Fill in data to a save_class
    template <typename save_class, typename T>
    void FillIn(PlayerId player_id, const CmdReceiver& receiver, T *game) const;

    void SaveSnapshot(serializer::saver &saver) const;
    void LoadSnapshot(serializer::loader &loader);

    // Compute the hash code.
    uint64_t CurrentHashCode() const;

    string PrintDebugInfo() const;

    string PrintPlayerInfo() const;
    ~GameEnv() { }

    // Some utility function to pick first from units in a group.
    static const Unit *PickFirstIdle(const vector<const Unit *> units, const CmdReceiver &receiver);
    static const Unit *PickFirst(const vector<const Unit *> units, const CmdReceiver &receiver, CmdType t) ;

    //用于锁定目标的方法
    string PrintUnitInfo() const;
    string PrintTargetsInfo(PlayerId player_id,UnitId radar_id = -1);
    void UpdateTargets(PlayerId player_id);  // 更新锁定名单
    bool Lock(PlayerId player_id,UnitId radar_id,UnitId target_id); //指定雷达锁定目标
    bool UnLock(PlayerId player_id,UnitId target_id);   //解锁一个目标
    bool CheckUnit(UnitId u_id,UnitType type);  // 检查一个目标是否存在且符合给定类型

   
};

class UnitIterator;

class GameEnvAspect {
public:
    GameEnvAspect(const GameEnv &env, PlayerId player_id)
        : _env(env), _player_id(player_id) {
    }

    bool FilterWithFOW(const Unit &u) const {
        return _player_id == INVALID || _env.GetPlayer(_player_id).FilterWithFOW(u);
    }
    // [TODO] This violates the behavior of Aspect. Will need to change. 
    const Units &GetAllUnits() const { return _env.GetUnits(); }
    const Player &GetPlayer() const { return _env.GetPlayer(_player_id); }
    const GameDef &GetGameDef() const { return _env.GetGameDef(); }

private:
    const GameEnv &_env;
    PlayerId _player_id;
};

class UnitIterator {
public:
    enum Type { ALL = 0, BUILDING, MOVING }; 

    UnitIterator(const GameEnvAspect &aspect, Type type)
        : _aspect(aspect), _type(type) {
            _it = _aspect.GetAllUnits().begin();
            next();
        }
    UnitIterator(const UnitIterator &i) 
        : _aspect(i._aspect), _type(i._type), _it(i._it) {
    }

    UnitIterator &operator ++() {
        ++ _it;
        next();
        return *this;
    }

    const Unit &operator *() {
        return *_it->second;
    }

    bool end() const { return _it == _aspect.GetAllUnits().end(); }

private:
    const GameEnvAspect &_aspect;
    Type _type;
    Units::const_iterator _it;

    void next() {
        while (_it != _aspect.GetAllUnits().end()) {
            const Unit &u = *_it->second;
            if (_aspect.FilterWithFOW(u)) {
                if (_type == ALL) break;

                bool is_building = _aspect.GetGameDef().IsUnitTypeBuilding(u.GetUnitType());
                if ((is_building && _type == BUILDING) || (! is_building && _type == MOVING)) break;
            }
            ++ _it;
        }
    }
};

// Fill in data to a save_class
template <typename save_class, typename T>
void GameEnv::FillIn(PlayerId player_id, const CmdReceiver& receiver, T *game) const {
    bool is_spectator = (player_id == INVALID);

    save_class::SetPlayerId(player_id, game);
    save_class::SetSpectator(is_spectator, game);

    if (is_spectator) {
        // Show all the maps.
        save_class::Save(*_map, game);
        for (size_t i = 0; i < _players.size(); ++i) {
            save_class::SaveStats(_players[i], game);
        }
    } else {
        // Show only visible maps
        // cout << "Save maps and stats" << endl << flush;
        save_class::SavePlayerMap(_players[player_id], game);
        save_class::SaveStats(_players[player_id], game);
    }

    // cout << "Save units" << endl << flush;
    GameEnvAspect aspect(*this, player_id);

    // Building iterator.
    UnitIterator iterator_build(aspect, UnitIterator::BUILDING);
    while (! iterator_build.end()) {
        save_class::Save(*iterator_build, &receiver, game);
        ++ iterator_build;
    }

    // Moving iterator.
    UnitIterator iterator_move(aspect, UnitIterator::MOVING);
    while (! iterator_move.end()) {
        save_class::Save(*iterator_move, &receiver, game);
        ++ iterator_move;
    }

    // cout << "Save bullet" << endl << flush;
    for (const auto& bullet : _bullets) {
        save_class::Save(bullet, game);
    }
}

#endif
