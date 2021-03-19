/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "state_feature.h"

static inline void accu_value(int idx, float val, std::map<int, std::pair<int, float> > &idx2record) {
    auto it = idx2record.find(idx);
    if (it == idx2record.end()) {
        idx2record.insert(make_pair(idx, make_pair(1, val)));
    } else {
        it->second.second += val;
        it->second.first ++;
    }
}

MCExtractorInfo MCExtractor::info_;   // leak?
MCExtractorUsageOptions MCExtractor::usage_;
bool MCExtractor::attach_complete_info_ = false;

void MCExtractor::SaveInfo(const RTSState &s, PlayerId player_id, GameState *gs) {
    gs->tick = s.GetTick();
    gs->winner = s.env().GetWinnerId();
    gs->terminal = s.env().GetTermination() ? 1 : 0;


    // 测试获取基地位置
    // UnitId baseId = s.env().FindClosestBase(player_id);
    //const Unit* base = s.env().GetUnit(s.env().FindClosestBase(player_id));
    //gs->base_x = s.env().GetUnit(s.env().FindClosestBase(player_id))->GetPointF().x;
    //gs->base_y = s.env().GetUnit(s.env().FindClosestBase(player_id))->GetPointF().y;
    // 测试获取基地位置
    
    // 设置奖励
    // 过程
    GameEnv& env_temp = const_cast<GameEnv&>(s.env()); // 需要用到GameEnv的方法
    Player& player_nn = env_temp.GetPlayer(player_id);
    
    gs->last_r = player_nn.GetPlayerReward(); 
   //std::cout<<"Reward: "<< gs->last_r<<" reward: "<<player_nn.GetReward()<<" last_reward: "<<player_nn.GetLastReward()<<std::endl;
    player_nn.UpdateLastReward();
    
    //std::cout<<"After Update reward: "<<player_nn.GetReward()<<" last_reward: "<<player_nn.GetLastReward()<<std::endl;

    // 结局
    int winner = s.env().GetWinnerId();
    if (winner != INVALID) {
      if (winner == player_id) gs->last_r += 5.0;
      else gs->last_r -= 5.0;
      // cout << "player_id: " << player_id << " " << (gs->last_r > 0 ? "Won" : "Lose") << endl;
    }
}



//==== Test===
// 所需求信息的大小 （后续改为参数设置）
/***
 * 全局信息      1 x 3
 * 保卫目标数据   1 x 3
 * 雷达数据      2 x 3
 * 防御塔数据    16 x 8
 * 敌人数据      20 x 7
*/
const int global_size = 3;  
const int base_size = 3;
const int num_radar = 2;
const int radar_size = 3;
const int num_tower = 20;
const int tower_size = 8;  
const int num_enemy = 20;
const int enemy_size = 7;

void MCExtractor::Extract(const RTSState &s, const Preload_Train &preload, PlayerId player_id, bool respect_fow, GameState *gs) {
   // 全局数据
   //printf("Extractor\n");
   const GameEnv &env = s.env();
   auto & s_global = gs->s_global;
   s_global.resize(global_size);
   std::fill(s_global.begin(),s_global.end(),0.0f);
   s_global[0] = gs->tick;
   extract_global(preload,s_global);

   // 基地数据
   auto & s_base = gs->s_base;
   s_base.resize(base_size);
   std::fill(s_base.begin(),s_base.end(),0.0f);
   extract_base(preload,s_base);

   //雷达数据
   auto & s_radar = gs->s_radar;
   s_radar.resize(num_radar * radar_size);
   std::fill(s_radar.begin(),s_radar.end(),0.0f);
   extract_radar(env,preload,s_radar);
   
   //防御塔数据
   auto & s_tower = gs->s_tower;
   s_tower.resize(num_tower * tower_size);
   std::fill(s_tower.begin(),s_tower.end(),0.0f);
   extract_tower(s.GetTick(),preload,s_tower);

   //敌人数据
   auto & s_enemy = gs->s_enemy;
   s_enemy.resize(num_enemy * enemy_size);
   std::fill(s_enemy.begin(),s_enemy.end(),0.0f);
   extract_enemy(env,player_id,preload,s_enemy);
   //printf("全局数据： Time: %f Num_Tower: %f Num_Radar: %f\n",gs->s_global[0],gs->s_global[1],gs->s_global[2]);

}


// 收集信息
void MCExtractor::extract_global(const Preload_Train &preload,std::vector<float>& s_global){
    if(s_global.size() != global_size){
        printf("Wrong size of s_global\n"); 
        return;
    }
    s_global[1] = preload.MyTroops()[MELEE_ATTACKER].size();  // 记录防御塔塔数量
    s_global[2] = preload.MyTroops()[RANGE_ATTACKER].size();  // 记录雷达数量
}

void MCExtractor::extract_base(const Preload_Train &preload,std::vector<float>& s_base){
    if(s_base.size() != base_size){
        printf("Wrong size of s_base\n"); 
        return;
    }
    if(preload.GetResult() == Preload_Train::NO_BASE) return;
    const Unit* _base = preload.MyTroops()[BASE][0]; //现在我们假定只有一个保卫目标
    if(_base != nullptr){
        s_base[0] = _base->GetPointF().x;  // 基地位置
        s_base[1] = _base->GetPointF().y;  
        s_base[2] = _base->GetProperty()._hp/100; //血量
    }
    
}

void MCExtractor::extract_radar(const GameEnv &env, const Preload_Train &preload,std::vector<float>& s_radar){
    GameEnv& env_temp = const_cast<GameEnv&>(env); // 需要用到GameEnv的方法
    if(s_radar.size() != num_radar * radar_size){
        printf("Wrong size of radar\n"); 
        return;
    }
    const auto &radar = preload.MyTroops()[RANGE_ATTACKER];
    if(radar.empty()){
        return;
    }
    int count = 0;  //计数
    int offset = 0;
    for(int i =0;i<radar.size();++i){  //在我们的设计中，最大雷达数量不应该超过我们设置的数量(我们需要掌握每个雷达的情况)
        const Unit* u_radar = radar[i];  
        if(u_radar != nullptr){
            offset = count * radar_size;
            s_radar[offset] =   u_radar->GetPointF().x;
            s_radar[offset+1] = u_radar->GetPointF().y;
            s_radar[offset+2] = env_temp.GerRadarRound(u_radar->GetId());
            ++count;
        }
    }
}


void MCExtractor::extract_tower(Tick tick,const Preload_Train &preload,std::vector<float>& s_tower){
    if(s_tower.size() != num_tower*tower_size ){
        printf("Wrong size of tower\n"); 
        return;
    }
    
    const auto &tower = preload.MyTroops()[MELEE_ATTACKER];
    if(tower.empty()){
        // printf("No Tower\n");
        return;
    }
    int count = 0;  //计数
    int offset = 0;
    for(int i =0;i<tower.size();++i){  //在我们的设计中，最大防御塔数量不应该超过我们设置的数量(我们需要掌握每个防御塔的情况)
        const Unit* u_tower = tower[i];
        if(u_tower != nullptr){
            offset = count * tower_size;
            s_tower[offset] = u_tower->GetPointF().x;
            s_tower[offset+1] = u_tower->GetPointF().y;
            s_tower[offset+2] = u_tower->GetProperty().round;
            s_tower[offset+3] = u_tower->GetProperty()._att_r;
            // vector_to_base
            float d_t_b = preload.DistanceToBase(u_tower->GetPointF() );
            PointF v_t_b = preload.VectorToBase(u_tower->GetPointF() );
            s_tower[offset+4] = v_t_b.x/d_t_b;  // dx
            s_tower[offset+5] = v_t_b.y/d_t_b;  // dy
            s_tower[offset+6] = d_t_b;  // distance_to_base
            s_tower[offset+7] = u_tower->GetProperty().CD(CD_ATTACK).Passed(tick)?1:0; // CD 0 -- 不可攻击 1 -- 可以攻击
            ++count;
        }
    }
}


void MCExtractor::extract_enemy(const GameEnv &env, PlayerId player_id, const Preload_Train &preload,std::vector<float>& s_enemy){
    if(s_enemy.size() != num_enemy*enemy_size ){
        printf("Wrong size of enemy\n"); 
        return;
    }
    
    const auto &enemy = preload.EnemyTroopsInRange();
    if(enemy.empty()){
        // printf("No Enemy In Sight\n");
        return;
    }
    int count = 0;  //计数
    int offset = 0;
    
    for(int i =0;i<enemy.size();++i){
        if(count>=num_enemy) break;  //最大可以记录num_enemy个敌人的信息
        const Unit* u_enemy = enemy[i].second;
        if(u_enemy != nullptr){
            //player.isUnitLocked(_target)
            offset = count * enemy_size;
            s_enemy[offset] = u_enemy->GetPointF().x;
            s_enemy[offset+1] = u_enemy->GetPointF().y;
            s_enemy[offset+2] = u_enemy->GetUnitType();
            s_enemy[offset+3] = env.isUnitLock(player_id,u_enemy->GetId())?1:0; // 是否已经被锁定
            // vector_to_base
            float d_t_b = enemy[i].first;
            PointF v_t_b = preload.VectorToBase(u_enemy->GetPointF() );
            s_enemy[offset+4] = v_t_b.x/d_t_b;  // dx
            s_enemy[offset+5] = v_t_b.y/d_t_b;  // dy
            s_enemy[offset+6] = d_t_b;  // distance_to_base
            ++count;
        }
    

    }
    
}

//===========


void MCExtractor::Extract(const RTSState &s, PlayerId player_id, bool respect_fow, std::vector<float> *state) {
    // [Channel, width, height]
    const int kTotalChannel = attach_complete_info_ ? 2 * info_.size() : info_.size();
    const GameEnv &env = s.env();
    const auto &m = env.GetMap();
    const int sz = kTotalChannel * m.GetXSize() * m.GetYSize();
    state->resize(sz);
    std::fill(state->begin(), state->end(), 0.0);

    extract(s, player_id, respect_fow, &(*state)[0]);
    if (attach_complete_info_) {
        extract(s, player_id, false, &(*state)[m.GetXSize() * m.GetYSize() * info_.size()]);
    }

}



void MCExtractor::extract(const RTSState &s, PlayerId player_id, bool respect_fow, float *state) {
    assert(player_id != INVALID);
   
    const GameEnv &env = s.env();
   
    const Extractor *ext_ut = info_.get("UnitType");
    const Extractor *ext_feature = info_.get("Feature");
    const Extractor *ext_resource = info_.get("Resource");

    // The following features are optional and might be nullptr.
    const Extractor *ext_hist_bin = info_.get("HistBin"); // leak?
    const Extractor *ext_ut_prev_seen = info_.get("UnitTypePrevSeen");
    const Extractor *ext_hist_bin_prev_seen = info_.get("HistBinPrevSeen");

    const int kAffiliation = ext_feature->Get(MCExtractorInfo::AFFILIATION);
    const int kChHpRatio = ext_feature->Get(MCExtractorInfo::HP_RATIO);
    const int kChBuildSinceDecay = ext_feature->Get(MCExtractorInfo::HISTORY_DECAY);

    const auto &m = env.GetMap();

    std::map<int, std::pair<int, float> > idx2record;

    PlayerId visibility_check = respect_fow ? player_id : INVALID;

    Tick tick = s.GetTick();

    GameEnvAspect aspect(env, visibility_check);
    UnitIterator unit_iter(aspect, UnitIterator::ALL);

    const Player &player = env.GetPlayer(player_id);

    float total_hp_ratio = 0.0;

    int myworker = 0;
    int mytroop = 0;
    int mybarrack = 0;
    float base_hp_level = 0.0;

    while (! unit_iter.end()) {
        const Unit &u = *unit_iter;
        int x = int(u.GetPointF().x);
        int y = int(u.GetPointF().y);
        float hp_level = u.GetProperty()._hp / (u.GetProperty()._max_hp + 1e-6);
        float build_since = 50.0 / (tick - u.GetBuiltSince() + 1);
        UnitType t = u.GetUnitType();

        bool self_unit = (u.GetPlayerId() == player_id);

        accu_value(_OFFSET(ext_ut->Get(t), x, y, m), 1.0, idx2record);

        // Self unit or enemy unit.
        // For historical reason, the flag of enemy unit = 2
        accu_value(_OFFSET(kAffiliation, x, y, m), (self_unit ? 1 : 2), idx2record);
        accu_value(_OFFSET(kChHpRatio, x, y, m), hp_level, idx2record);

        if (usage_.type >= BUILD_HISTORY) {
            accu_value(_OFFSET(kChBuildSinceDecay, x, y, m), build_since, idx2record);
            if (ext_hist_bin != nullptr) {
                int h_idx = ext_hist_bin->Get(tick - u.GetBuiltSince());
                accu_value(_OFFSET(h_idx, x, y, m), 1, idx2record);
            }
        }

        total_hp_ratio += hp_level;

        if (self_unit) {
            if (t == WORKER) myworker += 1;
            else if (t == MELEE_ATTACKER || t == RANGE_ATTACKER) mytroop += 1;
            else if (t == BARRACKS) mybarrack += 1;
            else if (t == BASE) base_hp_level = hp_level;
       }

        ++ unit_iter;
    }

    // Add seen objects.
    if (ext_ut_prev_seen != nullptr && ext_hist_bin_prev_seen != nullptr && usage_.type >= PREV_SEEN) {
        for (int x = 0; x < m.GetXSize(); ++x) {
            for (int y = 0; y < m.GetYSize(); ++y) {
                Loc loc = m.GetLoc(x, y);
                const Fog &f = player.GetFog(loc);
                if (usage_.type == ONLY_PREV_SEEN && f.CanSeeTerrain()) continue;
                for (const auto &u : f.seen_units()) {
                    UnitType t = u.GetUnitType();
                    int t_idx = ext_ut_prev_seen->Get(t);
                    accu_value(_OFFSET(t_idx, x, y, m), 1.0, idx2record);

                    int h_idx = ext_hist_bin_prev_seen->Get(tick - u.GetBuiltSince());
                    accu_value(_OFFSET(h_idx, x, y, m), 1, idx2record);
                }
            }
        }
    }

    for (const auto &p : idx2record) {
        state[p.first] = p.second.second / p.second.first;
    }

    myworker = min(myworker, 3);
    mytroop = min(mytroop, 5);
    mybarrack = min(mybarrack, 1);

    // Add resource layer for the current player.
    // const int c_idx = ext_resource->Get(player.GetResource());
    // const int c = _OFFSET(c_idx, 0, 0, m);
    // std::fill(state + c, state + c + m.GetXSize() * m.GetYSize(), 1.0);

    // if (usage_.type >= BUILD_HISTORY) {
    //     const int kChBaseHpRatio = ext_feature->Get(MCExtractorInfo::BASE_HP_RATIO);
    //     const int c = _OFFSET(kChBaseHpRatio, 0, 0, m);
    //     std::fill(state + c, state + c + m.GetXSize() * m.GetYSize(), base_hp_level);
    // }
}




// Preload


void Preload_Train::GatherInfo(const GameEnv& env, int player_id) {
    //cout << "GatherInfo(): player_id: " << player_id << endl;
    assert(player_id >= 0 && player_id < env.GetNumOfPlayers());
    collect_stats(env, player_id);
    const Player& player = env.GetPlayer(_player_id);
    if (!env.GetGameDef().HasBase()) return;
    if ( _my_troops[BASE].empty()) {
        //cout<<"No_BASE"<<endl;
        _result = NO_BASE;
        return;
    }
    // cout << "Base not empty" << endl << flush;
    _base = _my_troops[BASE][0];
    _base_id  = _base->GetId();
    _base_loc = _base->GetPointF();
    _base_hp  = _base->GetProperty()._hp;

    _result = OK;
}

bool cmp_enemy_by_distance(pair<float,const Unit*> p1, pair<float,const Unit*> p2){
    return p1.first<p2.first;
}
void Preload_Train::collect_stats(const GameEnv &env, int player_id) {
    // Clear all data
    //
    _my_troops.clear();
    _enemy_troops_in_range.clear();
    _all_my_troops.clear();
    _num_unit_type = env.GetGameDef().GetNumUnitType();

    // Initialize to a given size.
    _my_troops.resize(_num_unit_type);
    _result = NOT_READY;

    _player_id = player_id;

    // Collect ...
    const Units& units = env.GetUnits();
    const Player& player = env.GetPlayer(_player_id);

    // cout << "Looping over units" << endl << flush;

    // Get the information of all other troops.
    for (auto it = units.begin(); it != units.end(); ++it) {
        const Unit * u = it->second.get();
        if (u == nullptr) cout << "Unit cannot be nullptr" << endl << flush;

        if (u->GetPlayerId() == _player_id ) {  // 我方单位
            _my_troops[u->GetUnitType()].push_back(u);
           _all_my_troops.push_back(u);
        } else {  // 敌方单位
            if (player.FilterWithFOW(*u)) {
                if (u->GetUnitType() != RESOURCE) {
                    _enemy_troops_in_range.push_back(make_pair(DistanceToBase(u->GetPointF()) , u ));
                }
            }
        }
    }
    
    if(!_enemy_troops_in_range.empty()){
        // 按照距离目标的远近进行排序
        sort(_enemy_troops_in_range.begin(),_enemy_troops_in_range.end(),cmp_enemy_by_distance);
    }
}


PointF Preload_Train::VectorToBase(const PointF& u_loc) const{
    if(_result == Preload_Train::NO_BASE) return PointF();
    return PointF(_base_loc.x - u_loc.x, _base_loc.y - u_loc.y);
}

float Preload_Train::DistanceToBase(const PointF &u_loc) const{
    if(_result == Preload_Train::NO_BASE) return -1;
    return sqrt(PointF::L2Sqr(u_loc,BaseLoc()));
}



