/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#ifndef _MAP_H_
#define _MAP_H_

#include <functional>
#include <vector>
#include "common.h"
#include "locality_search.h"

struct MapSlot {
  // three layers, terrian, ground and air.
  Terrain type;
  int height;

  // Pre-computed distance map for fog of war computation (and path-planning).
  // sorted nearest neighbor after considering the map structure.
  std::vector<Loc> _nns;

  // Default constructor
  MapSlot() : type(NORMAL), height(0) {
  }

  SERIALIZER(MapSlot, type, height, _nns);
};

struct PlayerMapInfo {
    PlayerId player_id;
    Coord base_coord;
    Coord resource_coord;
    int initial_resource;

    SERIALIZER(PlayerMapInfo, player_id, base_coord, resource_coord, initial_resource);
};

// Map properties.
// Map location is an integer.
class RTSMap {
private:
  vector<MapSlot> _map;

  // Size of the map.
  int _m, _n, _level;

  // Initial players' info
  vector<PlayerMapInfo> _infos;

  // Locality search.
  LocalitySearch<UnitId> _locality;

private:
  void reset_intermediates();
  void load_default_map();
  void precompute_all_pair_distances();

  bool find_two_nearby_empty_slots(const std::function<uint16_t (int)>& f, int *x1, int *y1, int *x2, int *y2, int i) const;

public:
  // Load map from a file.
  RTSMap();
  bool GenerateMap(const std::function<uint16_t (int)>& f, int nImpassable, int num_player, int init_resource);
  bool LoadMap(const std::string &filename);
  bool GenerateImpassable(const std::function<uint16_t(int)>& f, int nImpassable);

  // TODO: move this to game_TD
  bool GenerateTDMaze(const std::function<uint16_t(int)>& f);


  const vector<PlayerMapInfo> &GetPlayerMapInfo() const { return _infos; }
  void ClearMap() { _infos.clear(); _locality.Clear();}

  const MapSlot &operator()(const Loc& loc) const { return _map[loc]; }
  MapSlot &operator()(const Loc& loc) { return _map[loc]; }

  int GetXSize() const { return _m; }
  int GetYSize() const { return _n; }
  int GetPlaneSize() const { return _m * _n; }

  // TODO: move this to game_TD
  bool CanBuildTower(const PointF &p, UnitId id_exclude) const {
      Coord c = p.ToCoord();
      if (! IsIn(c)) return false;

      Loc loc = GetLoc(c);
      const MapSlot &s = _map[loc];
      // cannot block the path
      if (s.type == NORMAL) return false;

      // [TODO] Add object radius here.
      return _locality.IsEmpty(p, kUnitRadius, id_exclude);
  }

  bool CanPass(const PointF &p, UnitId id_exclude, bool check_locality = true) const {
      Coord c = p.ToCoord();
      if (! IsIn(c)) return false;

      Loc loc = GetLoc(c);
      const MapSlot &s = _map[loc];
      if (s.type == IMPASSABLE) return false;

      // [TODO] Add object radius here.
      if (check_locality)
        return _locality.IsEmpty(p, kUnitRadius, id_exclude);
      else
        return true;
  }

  bool CanPass(const Coord &c, UnitId id_exclude, bool check_locality = true) const {
      if (! IsIn(c)) return false;

      Loc loc = GetLoc(c);
      const MapSlot &s = _map[loc];
      if (s.type == IMPASSABLE) return false;

      // [TODO] Add object radius here.
      if (check_locality)
        return _locality.IsEmpty(PointF(c.x, c.y), kUnitRadius, id_exclude);
      else
        return true;
  }

  bool IsLinePassable(const PointF &s, const PointF &t) const {
      LineResult result;
      UnitId block_id = INVALID;
      return _locality.LinePassable(s, t, kUnitRadius, 1e-4, &block_id, &result);
  }

  // Move a unit to next_loc;
  bool MoveUnit(const UnitId &id, const PointF& new_loc);
  bool AddUnit(const UnitId &id, const PointF& new_loc);
  bool RemoveUnit(const UnitId &id);

  // Coord transfer.
  string PrintCoord(Loc loc) const;
  Coord GetCoord(Loc loc) const;
  Loc GetLoc(const Coord& c) const;
  Loc GetLoc(int x, int y, int z = 0) const;

  // Get sight from the current location.
  vector<Loc> GetSight(const Loc& loc, int range) const;

  bool IsIn(Loc loc) const { return loc >= 0 && loc < _m * _n * _level; }
  bool IsIn(int x, int y) const { return x >= 0 && x < _m && y >= 0 && y < _n; }
  bool IsIn(const Coord &c, int margin = 0) const { return c.x >= margin && c.x < _m - margin && c.y >= margin && c.y < _n - margin && c.z >= 0 && c.z < _level; }
  bool IsIn(const PointF &p, int margin = 0) const { return IsIn(p.ToCoord(), margin); }

  // Get the nearest location.
  Loc GetLoc(const PointF& p) const { return GetLoc(p.ToCoord()); }

  UnitId GetClosestUnitId(const PointF& p, float max_r = 1e38) const;
  set<UnitId> GetUnitIdInRegion(const PointF &left_top, const PointF &right_bottom) const;

  // Draw the map
  string Draw() const;

  string PrintDebugInfo() const;

  SERIALIZER(RTSMap, _m, _n, _level, _map, _infos, _locality);
};

#endif
