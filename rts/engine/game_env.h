/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _GAME_DEV_H_
#define _GAME_DEV_H_

#include "cmd_receiver.h"
#include "unit.h"
#include "bullet.h"
#include "map.h"
#include "player.h"
#include <random>
#include "lua.hpp"
#include "selene.h"


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

    mutable sel::State _sel_state;

    bool _use_sel = true;

public:
    class UnitIterator {
        private:
            Units::const_iterator _it;
            const GameEnv *_env;
            PlayerId _player_id;
            bool _output_building;
            bool _output_moving;

            void next() {
                while (_it != _env->_units.end()) {
                    const Unit &u = *_it->second;
                    if (_player_id == INVALID || _env->_players[_player_id].FilterWithFOW(u)) {
                        bool is_building = _env->_gamedef.IsUnitTypeBuilding(u.GetUnitType());
                        if ((is_building && _output_building) || (! is_building && _output_moving)) break;
                    }
                    ++ _it;
                }
            }

        public:
            UnitIterator(const GameEnv *env, PlayerId player_id, bool output_building, bool output_moving)
                : _env(env), _player_id(player_id), _output_building(output_building), _output_moving(output_moving) {
                _it = _env->_units.begin();
                next();
            }
            UnitIterator &operator ++() {
                ++ _it;
                next();
                return *this;
            }

            const Unit &operator *() {
                return *_it->second;
            }

            bool end() const { return _it == _env->_units.end(); }
    };

    GameEnv();

    sel::State &GetSelState() const {
        return _sel_state;
    }

    bool GetUseSel() const {
        return _use_sel;
    }

    void SetUseSel(bool use_sel) {
        _use_sel = use_sel;
    }

    void Visualize() const;

    // Remove all players.
    void ClearAllPlayers();

    // Reset RTS game to starting position
    void Reset();

    // Add and remove players.
    void AddPlayer(PlayerPrivilege pv);
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

    const Player &GetPlayer(PlayerId player_id) const { return _players[player_id]; }
    Player &GetPlayer(PlayerId player_id) { return _players[player_id]; }

    // Add and remove units.
    bool AddUnit(Tick tick, UnitType type, const PointF &p, PlayerId player_id);
    bool RemoveUnit(const UnitId &id);

    void AddBullet(const Bullet &b) { _bullets.push_back(b); }

    // Check if one player's base has been destroyed.
    PlayerId CheckBase(UnitType base_type) const;

    // Getter and setter for winner_id, termination.
    void SetWinnerId(PlayerId winner_id) { _winner_id = winner_id; }
    PlayerId GetWinnerId() const { return _winner_id; }
    void SetTermination() { _terminated = true; }
    bool GetTermination() const { return _terminated; }

    // Compute bullets cmds.
    void Forward(CmdReceiver *receiver);
    void ComputeFOW();

    UnitIterator GetUnitIterator(PlayerId player_id) const { return UnitIterator(this, player_id, true, true); }
    UnitIterator GetUnitBuildingIterator(PlayerId player_id) const { return UnitIterator(this, player_id, true, false); }
    UnitIterator GetUnitMovingIterator(PlayerId player_id) const { return UnitIterator(this, player_id, false, true); }

    // Fill in metadata to a save_class
    template <typename save_class, typename T>
    void FillHeader(const CmdReceiver& receiver, T *game) const {
        save_class::SetTick(receiver.GetTick(), game);
        save_class::SetWinner(_winner_id, game);
        save_class::SetTermination(_terminated, game);
    }

    // Fill in data to a save_class
    template <typename save_class, typename T>
    void FillIn(PlayerId player_id, const CmdReceiver& receiver, T *game) const {
        bool is_spectator = (player_id == INVALID);
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
        auto iterator = GetUnitBuildingIterator(player_id);
        while (! iterator.end()) {
            save_class::Save(*iterator, receiver, game);
            ++ iterator;
        }

        iterator = GetUnitMovingIterator(player_id);
        while (! iterator.end()) {
            save_class::Save(*iterator, receiver, game);
            ++ iterator;
        }

        // cout << "Save bullet" << endl << flush;
        for (const auto& bullet : _bullets) {
            save_class::Save(bullet, game);
        }
    }

    void SaveSnapshot(serializer::saver &saver) const;
    void LoadSnapshot(serializer::loader &loader);

    // Compute the hash code.
    uint64_t CurrentHashCode() const;

    string PrintDebugInfo() const;
    ~GameEnv() { }

    // Some utility function to pick first from units in a group.
    static const Unit *PickFirstIdle(const vector<const Unit *> units, const CmdReceiver &receiver);
    static const Unit *PickFirst(const vector<const Unit *> units, const CmdReceiver &receiver, CmdType t) ;
};

#endif
