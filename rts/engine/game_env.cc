/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "game_env.h"
#include "cmd.h"

GameEnv::GameEnv() {
    // Load the map.
    // std::cout<<"-------GameEnv_map-----------"<<std::endl;
    _map = unique_ptr<RTSMap>(new RTSMap());
     
    _game_counter = -1;
    Reset();
    //std::cout<<"-------GameEnv Finish-----------"<<std::endl;
}

void GameEnv::Visualize() const {
    for (const auto &player : _players) {
        std::cout << player.PrintInfo() << std::endl;
    }
    // No FoW, everything.
    GameEnvAspect aspect(*this, INVALID);
    UnitIterator unit_iter(aspect, UnitIterator::ALL);
    while (! unit_iter.end()) {
        const Unit &u = *unit_iter;
        std::cout << u.PrintInfo(*_map) << std::endl;
        ++ unit_iter;
    }
}

void GameEnv::ClearAllPlayers() {
    _players.clear();
}

void GameEnv::Reset() {
    //std::cout<<"-------Reset-----------"<<std::endl;
    _map->ClearMap();
    _next_unit_id = 0;
    _winner_id = INVALID;
    _terminated = false;
    _game_counter ++;
    _units.clear();
    _bullets.clear();
    _trace.clear();
    
    for (auto& player : _players) {
        player.ClearCache();
    }
}

void GameEnv::AddPlayer(const std::string &name, PlayerPrivilege pv) {
    _players.emplace_back(*_map, name, _players.size());
    _players.back().SetPrivilege(pv);
}

void GameEnv::RemovePlayer() {
    _players.pop_back();
}

void GameEnv::SaveSnapshot(serializer::saver &saver) const {
    serializer::Save(saver, _next_unit_id);

    saver << _map;
    saver << _units;
    saver << _bullets;
    saver << _players;
    saver << _winner_id;
    saver << _terminated;
}

void GameEnv::LoadSnapshot(serializer::loader &loader) {
    serializer::Load(loader, _next_unit_id);

    loader >> _map;
    loader >> _units;
    loader >> _bullets;
    loader >> _players;
    loader >> _winner_id;
    loader >> _terminated;

    for (auto &player : _players) {
        player.ResetMap(_map.get());
    }
}

// Compute the hash code.
uint64_t GameEnv::CurrentHashCode() const {
    uint64_t code = 0;
    for (auto it = _units.begin(); it != _units.end(); ++it) {
        serializer::hash_combine(code, it->first);
        serializer::hash_combine(code, *it->second);
        // cout << "Unit: " << it->first << ": #hash = " << this_code << ", " << it->second->GetProperty().CD(CD_ATTACK).PrintInfo() << endl;
        // code ^= this_code;
    }
    // Players.
    for (const auto &player : _players) {
        serializer::hash_combine(code, player);
    }
    return code;
}

bool GameEnv::AddUnit(Tick tick, UnitType type, const PointF &p, PlayerId player_id, UnitId& u_id) {
    // Check if there is any space.
    if (!_gamedef.CheckAddUnit(_map.get(), type, p)) return false;
    // cout << "Actual adding unit." << endl;

    UnitId new_id = Player::CombinePlayerId(_next_unit_id, player_id);
    Unit *new_unit = new Unit(tick, new_id, type, p, _gamedef.unit(type)._property);
    _units.insert(make_pair(new_id, unique_ptr<Unit>(new_unit)));
    _map->AddUnit(new_id, p);
    
    u_id = new_id;
    //cout<<"AddUnit u_id: "<<u_id<<endl;

    _next_unit_id ++;
    return true;
}

bool GameEnv::RemoveUnit(const UnitId &id) {
    auto it = _units.find(id);
    if (it == _units.end()) return false;
    _units.erase(it);

    _map->RemoveUnit(id);
    return true;
}

// 找到最近的基地
UnitId GameEnv::FindClosestBase(PlayerId player_id) const {
    // Find closest base. [TODO]: Not efficient here.
    for (auto it = _units.begin(); it != _units.end(); ++it) {
        const Unit *u = it->second.get();
        if ((u->GetUnitType() == BASE || u->GetUnitType() == FLAG_BASE) && u->GetPlayerId() == player_id) {
            return u->GetId();
        }
    }
    return INVALID;
}

PlayerId GameEnv::CheckBase(UnitType base_type) const{
    PlayerId last_player_has_base = INVALID;
    for (auto it = _units.begin(); it != _units.end(); ++it) {
        const Unit *u = it->second.get();
        if (u->GetUnitType() == base_type) {
            if (last_player_has_base == INVALID) {
                last_player_has_base = u->GetPlayerId();
            } else if (last_player_has_base != u->GetPlayerId()) {
                // No winning.
                last_player_has_base = INVALID;
                break;
            }
        }
    }
    return last_player_has_base;
}
 PlayerId GameEnv::CheckWinner(UnitType base_type) const {
     PlayerId player_id  = 0;
     PlayerId enemy_id = 1;
     PlayerId base_player_id = INVALID;
     bool hasEnemyUnit = false;
     if(_units.size()== 0){
        return INVALID;
     }
     //std::cout<<"_units.size() : "<<_units.size()<<std::endl;
     for (auto it = _units.begin(); it != _units.end(); ++it){
          const Unit *u = it->second.get();
           if (u->GetUnitType() == base_type){
               base_player_id = u->GetPlayerId();
           }
           if(!hasEnemyUnit){  //如果还没有检测到敌方单位，持续判断
               if(((u->GetUnitType() == WORKER || u->GetUnitType() == BARRACKS || u->GetUnitType() == BASE) && u->GetPlayerId() == enemy_id))
                    hasEnemyUnit = true;
           }        
     }
     if(base_player_id == INVALID){  //只有玩家有基地，基地被摧毁则认为玩家失败，游戏结束
        return enemy_id;  
     }else if(!hasEnemyUnit){  //玩家基地存在，不存在敌方单位，玩家胜利
        return player_id;   
     } 
     return INVALID;    // 游戏还未结束
 }




bool GameEnv::FindEmptyPlaceNearby(const PointF &p, int l1_radius, PointF *res_p) const {
    // Find an empty place by simple local grid search.
    const int margin = 2;
    const int cx = _map->GetXSize() / 2;
    const int cy = _map->GetYSize() / 2;
    int sx = p.x < cx ? -1 : 1;
    int sy = p.y < cy ? -1 : 1;

    for (int dx = -sx * l1_radius; dx != sx * l1_radius + sx; dx += sx) {
        for (int dy = -sy * l1_radius; dy != sy * l1_radius + sy; dy += sy) {
            PointF new_p(p.x + dx, p.y + dy);
            if (_map->CanPass(new_p, INVALID) && _map->IsIn(new_p, margin)) {
                // It may not be a good strategy, though.
                *res_p = new_p;
                return true;
            }
        }
    }
    return false;
}

bool GameEnv::FindBuildPlaceNearby(const PointF &p, int l1_radius, PointF *res_p) const {
    // Find an empty place by simple local grid search.
    for (int dx = -l1_radius; dx <= l1_radius; dx ++) {
        for (int dy = -l1_radius; dy <= l1_radius; dy ++) {
            PointF new_p(p.x + dx, p.y + dy);
            if (_map->CanBuildTower(new_p, INVALID)) {
                *res_p = new_p;
                return true;
            }
        }
    }
    return false;
}

// given a set of units and a target point, a distance, find closest place to go to to maintain the distance.
// can be used by hit and run or scout.
bool GameEnv::FindClosestPlaceWithDistance(const PointF &p, int dist,
  const vector<const Unit *>& units, PointF *res_p) const {
  const RTSMap &m = *_map;
  vector<Loc> distances(m.GetXSize() * m.GetYSize());
  vector<Loc> current;
  vector<Loc> nextloc;
  for (auto unit : units) {
      Loc loc = m.GetLoc(unit->GetPointF().ToCoord());
      distances[loc] = 0;
      current.push_back(loc);
  }
  const int dx[] = { 1, 0, -1, 0 };
  const int dy[] = { 0, 1, 0, -1 };
  for (int d = 1; d <= dist; d++) {
      for (Loc loc : current) {
          Coord c_curr = m.GetCoord(loc);
          for (size_t i = 0; i < sizeof(dx) / sizeof(int); ++i) {
              Coord next(c_curr.x + dx[i], c_curr.y + dy[i]);
              if (_map->CanPass(next, INVALID) && _map->IsIn(next)) {
                  Loc l_next = m.GetLoc(next);
                  if (distances[l_next] == 0 || distances[l_next] > d) {
                    nextloc.push_back(l_next);
                    distances[l_next] = d;
                  }
              }
          }
      }
      current = nextloc;
      nextloc.clear();
  }

  float closest = m.GetXSize() * m.GetYSize();
  bool found = false;
  int k = 0;
  for (Loc loc : current) {
      k++;
      PointF pf = PointF(m.GetCoord(loc));
      float dist_sqr = PointF::L2Sqr(pf, p);
      if (closest > dist_sqr) {
          *res_p = pf;
          closest = dist_sqr;
          found = true;
      }
  }
  return found;
}

void GameEnv::Forward(CmdReceiver *receiver) {
    // Compute all bullets.
    set<int> done_bullets;
    for (size_t i = 0; i < _bullets.size(); ++i) {
        CmdBPtr cmd = _bullets[i].Forward(*_map, _units);
        if (cmd.get() != nullptr) {
            // Note that this command is special. It should not be recorded in
            // the cmd_history.
            receiver->SetSaveToHistory(false);
            receiver->SendCmd(std::move(cmd));
            receiver->SetSaveToHistory(true);
        }
        if (_bullets[i].IsDead()) done_bullets.insert(i);
    }

    // Remove bullets that are done.
    // Need to traverse in the reverse order.
    for (set<int>::reverse_iterator it = done_bullets.rbegin(); it != done_bullets.rend(); ++it) {
        unsigned int idx = *it;
        if (idx < _bullets.size() - 1) {
            swap(_bullets[idx], _bullets.back());
        }
        _bullets.pop_back();
    }
}

void GameEnv::ComputeFOW() {
    // Compute FoW.
    for (Player &p : _players) {
        p.ComputeFOW(_units);
    }
}

int GameEnv::GetPrevSeenCount(PlayerId player_id) const {
    GameEnvAspect aspect(*this, player_id);
    const Player &player = aspect.GetPlayer();

    set<UnitId> unit_ids;

    // cout << "GetPrevSeenCount: " << endl;

    for (int x = 0; x < _map->GetXSize(); ++x) {
        for (int y = 0; y < _map->GetYSize(); ++y) {
            Loc loc = _map->GetLoc(x, y);
            const Fog &f = player.GetFog(loc);
            if (f.CanSeeTerrain()) continue;
            for (const Unit &u : f.seen_units()) {
                // cout << u.PrintInfo(*_map) << endl;
                unit_ids.insert(u.GetId());
            }
        }
    }
    return unit_ids.size();
}

bool GameEnv::GenerateMap(int num_obstacles, int init_resource) {
    return _map->GenerateMap(GetRandomFunc(), num_obstacles, _players.size(), init_resource);
}

bool GameEnv::GenerateImpassable(int num_obstacles) {
    return _map->GenerateImpassable(GetRandomFunc(), num_obstacles);
}

bool GameEnv::GenerateTDMaze() {
    return _map->GenerateTDMaze(GetRandomFunc());
}

const Unit *GameEnv::PickFirstIdle(const vector<const Unit *> units, const CmdReceiver &receiver) {
    return PickFirst(units, receiver, INVALID_CMD);
}

const Unit *GameEnv::PickFirst(const vector<const Unit *> units, const CmdReceiver &receiver, CmdType t) {
    for (const auto *u : units) {
        const CmdDurative *cmd = receiver.GetUnitDurativeCmd(u->GetId());
        if ( (t == INVALID_CMD && cmd == nullptr) || (cmd != nullptr && cmd->type() == t)) return u;
    }
    return nullptr;
}

string GameEnv::PrintDebugInfo() const {
    stringstream ss;
    ss << "Game #" << _game_counter << endl;
    for (const auto& player : _players) {
        ss << "Player " << player.GetId() << endl;
        ss << player.PrintHeuristicsCache() << endl;
    }

    ss << _map->Draw() << endl;
    ss << _map->PrintDebugInfo() << endl;
    return ss.str();
}

string GameEnv::PrintPlayerInfo() const {
    stringstream ss;
    for (const auto& player : _players) {
        ss << "Player " << player.GetId() << endl;
        ss << player.PrintInfo()<< endl;
    }

    //ss << _map->Draw() << endl;
    //ss << _map->PrintDebugInfo() << endl;
    return ss.str();
}


// =============追踪目标的方法==============
string GameEnv::PrintUnitInfo() const {
    stringstream ss;
    ss<<"UnitInfo: "<<endl;
    for(auto& i : _units){
        ss<<"UnitId: "<<i.first<<" UnitType: "<<i.second->GetUnitType()<<" Pointf: "<<"Player_id: "<<i.second->GetPlayerId()<<endl;
    }
    return ss.str();
}

string GameEnv::PrintTargetsInfo(PlayerId player_id,UnitId radar_id) {
    // ToDO  查询前要先更新
    UpdateTargets(player_id);
    stringstream ss;
    if(radar_id != -1){
        //_units.find(radar_id) == _units.end() || _units[radar_id]->GetUnitType() != RANGE_ATTACKER
      if( !CheckUnit(radar_id,RANGE_ATTACKER) || _units[radar_id]->GetPlayerId() != player_id){   
            ss<<"---------Failed Invalid RadarId "<<radar_id<<"------------------";
            return ss.str();
        }
    }
    for( auto& player : _players){
        if(player.GetId() == player_id ){
            ss<<player.PrintTargetInfo(radar_id);
            break;
        }
    }
    return ss.str();
}


void GameEnv::UpdateTargets(PlayerId player_id){
    for(auto& player : _players){
        if(player.GetId() == player_id){
            Targets _targets = player.GetTargets();
            for(auto r : _targets){
                if(_units.find(r.first) == _units.end()){  // 雷达失活
                    player.RemoveRadar(r.first);
                    continue;
                }
                for(auto u : r.second){  // 遍历雷达的所有单位
                   // 移除所有失活或者离开FOW的单位
                   if(_units.find(u) == _units.end()){
                       player.RemoveUnit(u);
                       continue;
                   }
                   // 判断单位是否离开玩家FOW
                   const Unit * target = GetUnit(u);
                   if(!player.FilterWithFOW(*target))
                      player.RemoveUnit(u);
                }
            }
            break;
        }
    }
}

bool GameEnv::CheckUnit(UnitId u_id,UnitType type){
    return (_units.find(u_id) != _units.end() && _units[u_id]->GetUnitType() == type);  // 单位存在且类型相符
}
bool GameEnv::Lock(PlayerId player_id,UnitId radar_id,UnitId target_id){
    UpdateTargets(player_id);
    // 判断目标是否合法
    if(!CheckUnit(target_id,WORKER) && !CheckUnit(target_id,BARRACKS) && !CheckUnit(target_id,BASE)){ //如果目标不存在或不是导弹、飞机
        cout<<"--------Invalid Target------------"<<endl;
        return false;
    }
    // 判断雷达是否合法
    if(!CheckUnit(radar_id,RANGE_ATTACKER) || _units[radar_id]->GetPlayerId()!= player_id){
        cout<<"-----------Invalid Radar--------------"<<endl;
        return false;
    }
    for(auto& player : _players){
        if(player.GetId() == player_id){
            player.AddUnit(radar_id,target_id); //锁定目标
            return true;
        }
    }
    return false;
}

bool GameEnv::UnLock(PlayerId player_id,UnitId target_id){
    cout<<"Unlock---UnitId:"<<target_id<<endl;
    UpdateTargets(player_id);
    for(auto & player: _players){
        if(player.GetId() == player_id){
            return player.RemoveUnit(target_id);
            
        }
    }
    return false;
}


 UnitId GameEnv::FindUnitsInK(PlayerId player_id,int k) const{
     UnitId id = INVALID;
     for (auto it = _units.begin(); it != _units.end(); ++it){
          const Unit *u = it->second.get();
          if(u->GetPlayerId() == player_id && u->GetUnitType() != BASE){  // 是玩家单位且不是基地
               if(--k <= 0){
                   id = u->GetId();
                   break;
               }
          }
     }
     return id;
 }

 




