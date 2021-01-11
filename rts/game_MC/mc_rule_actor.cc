/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "mc_rule_actor.h"

bool MCRuleActor::ActByState2(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds) {
  //  cout<<"ActByState2"<<endl;
    assigned_cmds->clear();
    *state_string = "";

    RegionHist hist;

    // Then loop over all my troops to run.
    const auto& all_my_troops = _preload.AllMyTroops();
    for (const Unit *u : all_my_troops) {
        // Get the bin id.
        act_per_unit(env, u, &state[0], &hist, state_string, assigned_cmds);
    }

    return true;
}

bool MCRuleActor::ActByState(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds) {
    // Each unit can only have one command. So we have this map.
    // cout << "Enter ActByState" << endl << flush;
    //cout<<"ActByState"<<endl;
    assigned_cmds->clear();
    *state_string = "NOOP";

    // Build workers.
    if (state[STATE_BUILD_WORKER]) {
        *state_string = "Build worker..NOOP";
        const Unit *base = _preload.Base();
        if (IsIdle(*_receiver, *base)) {
            if (_preload.BuildIfAffordable(WORKER)) {
                *state_string = "Build worker..Success";
                store_cmd(base, _B(WORKER), assigned_cmds);
            }
        }
    }

    const auto& my_troops = _preload.MyTroops();

    // Ask workers to gather.
    for (const Unit *u : my_troops[WORKER]) {
        if (IsIdle(*_receiver, *u)) {
            // Gather!
            store_cmd(u, _preload.GetGatherCmd(), assigned_cmds);
        }
    }

    // 进攻
    if (state[STATE_BUILD_BARRACK]) {
        //cout<<"Normal STATE"<<endl;
        *state_string = "Build barracks..NOOP";
        //cout<<"STATE_BUILD_BARRACK"<<endl;
        
        // if (_preload.Affordable(BARRACKS)) {
        //     // cout << "Building barracks!" << endl;
        //     const Unit *u = GameEnv::PickFirst(my_troops[WORKER], *_receiver, GATHER);
        //     if (u != nullptr) {
        //         CmdBPtr cmd = _preload.GetBuildBarracksCmd(env);
        //         if (cmd != nullptr) {
        //             *state_string = "Build barracks..Success";
        //             store_cmd(u, std::move(cmd), assigned_cmds);
        //             _preload.Build(BARRACKS);
        //         }
        //     }
        // }
        if(_preload.HavePlane()){
            const Unit *u = GameEnv::PickFirstIdle(my_troops[WORKER], *_receiver);
            if(u!=nullptr){
                CmdBPtr cmd = _preload.GetMOVECmd();
                if (cmd != nullptr) {
                    *state_string = "Move..Success";
                    store_cmd(u, std::move(cmd), assigned_cmds);
                    //_preload.Build(BARRACKS);
                }
            }
        }

        for(const Unit* u : my_troops[WORKER]){
            //cout<<"curr distance"<<PointF::L2Sqr(u->GetPointF(),_preload.GetEnemyBaseLoc())<<endl;
            if(PointF::L2Sqr(u->GetPointF(),_preload.GetEnemyBaseLoc()) < 81.0f && u->GetProperty().round  > 1 ){
               //cout<<u->GetId()<<" Emit first Rocket"<<endl;
                auto cmd = _preload.GetAttackEnemyBaseCmd();
                store_cmd(u, std::move(cmd), assigned_cmds);
            }else if(PointF::L2Sqr(u->GetPointF(),_preload.GetEnemyBaseLoc()) < 25.0f && u->GetProperty().round  > 0){
                //cout<<u->GetId()<<" Emit second Rocket"<<endl;
                auto cmd = _preload.GetAttackEnemyBaseCmd();
                store_cmd(u, std::move(cmd), assigned_cmds);
            }
        }

        
    }

    // If we have barracks with resource, build troops.
    if (state[STATE_BUILD_MELEE_TROOP]) {
        *state_string = "Build Melee Troop..NOOP";
        if (_preload.Affordable(MELEE_ATTACKER)) {
            const Unit *u = GameEnv::PickFirstIdle(my_troops[BARRACKS], *_receiver);
            if (u != nullptr) {
                *state_string = "Build Melee Troop..Success";
                store_cmd(u, _B(MELEE_ATTACKER), assigned_cmds);
                _preload.Build(MELEE_ATTACKER);
            }
        }
    }

    if (state[STATE_BUILD_RANGE_TROOP]) {
        *state_string = "Build Range Troop..NOOP";
        if (_preload.Affordable(RANGE_ATTACKER)) {
            const Unit *u = GameEnv::PickFirstIdle(my_troops[BARRACKS], *_receiver);
            if (u != nullptr) {
                *state_string = "Build Range Troop..Success";
                store_cmd(u, _B(RANGE_ATTACKER), assigned_cmds);
                _preload.Build(RANGE_ATTACKER);
            }
        }
    }

    if (state[STATE_ATTACK]) {
        *state_string = "Attack..Normal";
        // 飞机攻击基地
         vector<const Unit*>  units;
        for (const Unit* u : my_troops[WORKER]){
             // 距离和弹药满足
            if(PointF::L2Sqr(u->GetPointF(),_preload.GetEnemyBaseLoc()) < u->GetProperty()._att_r *  u->GetProperty()._att_r && u->GetProperty().round > 0){
                units.push_back(u);
            }
        }
        
        if(units.size() > 0){
            auto cmd = _preload.GetAttackEnemyBaseCmd();
            batch_store_cmds(units, cmd, false, assigned_cmds);
        }
        
        
        // batch_store_cmds(my_troops[MELEE_ATTACKER], cmd, false, assigned_cmds);
        // batch_store_cmds(my_troops[RANGE_ATTACKER], cmd, false, assigned_cmds);
    }

    const auto& enemy_troops = _preload.EnemyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    const auto& all_my_troops = _preload.AllMyTroops();
    const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();

    if (state[STATE_HIT_AND_RUN]) {
        *state_string = "Hit and run";
        // cout << "Enter hit and run procedure" << endl << flush;
        if (enemy_troops[MELEE_ATTACKER].empty() && enemy_troops[RANGE_ATTACKER].empty() && ! enemy_troops[WORKER].empty()) {
            // cout << "Enemy only have worker" << endl << flush;
            for (const Unit *u : my_troops[RANGE_ATTACKER]) {
                hit_and_run(env, u, enemy_troops[WORKER], assigned_cmds);
            }
        }
        if (! enemy_troops[MELEE_ATTACKER].empty()) {
            // cout << "Enemy only have malee attacker" << endl << flush;
            for (const Unit *u : my_troops[RANGE_ATTACKER]) {
                hit_and_run(env, u, enemy_troops[MELEE_ATTACKER], assigned_cmds);
            }
        }
        if (! enemy_troops[RANGE_ATTACKER].empty()) {
            auto cmd = _A(enemy_troops[RANGE_ATTACKER][0]->GetId());
            batch_store_cmds(my_troops[MELEE_ATTACKER], cmd, false, assigned_cmds);
            batch_store_cmds(my_troops[RANGE_ATTACKER], cmd, false, assigned_cmds);
        }
    }

    if (state[STATE_ATTACK_IN_RANGE]) {  // 向目标发射导弹
         *state_string = "Attack enemy in range..NOOP";
    //   if (! enemy_troops_in_range.empty()) {
    //     *state_string = "Attack enemy in range..Success";
    //     auto cmd = _A(enemy_troops_in_range[0]->GetId());
    //     batch_store_cmds(my_troops[MELEE_ATTACKER], cmd, false, assigned_cmds);
    //     batch_store_cmds(my_troops[RANGE_ATTACKER], cmd, false, assigned_cmds);
    //   }
        if(! enemy_troops_in_range.empty()){
            
        }
    }

    if (state[STATE_DEFEND]) {
      // Group Retaliation. All troops attack.
      *state_string = "Defend enemy attack..NOOP";

      const Unit *enemy_at_resource = _preload.EnemyAtResource();
      if (enemy_at_resource != nullptr) {
          *state_string = "Defend enemy attack..Success";
          batch_store_cmds(all_my_troops, _A(enemy_at_resource->GetId()), true, assigned_cmds);
      }

      const Unit *enemy_at_base = _preload.EnemyAtBase();
      if (enemy_at_base != nullptr) {
          *state_string = "Defend enemy attack..Success";
          batch_store_cmds(all_my_troops, _A(enemy_at_base->GetId()), true, assigned_cmds);
      }

      if (! enemy_attacking_economy.empty()) {
        *state_string = "Defend enemy attack..Success";
        auto it = enemy_attacking_economy.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds(all_my_troops, cmd, true, assigned_cmds);
      }
    }
    return true;
}

bool MCRuleActor::GetActSimpleState(vector<int>* state) {
    //std::cout<<"Player "<< _player_id << " GetActSimpleState"<<std::cout;
    vector<int> &_state = *state;

    const auto& my_troops = _preload.MyTroops();
    //const auto& cnt_under_construction = _preload.CntUnderConstruction();
    const auto& enemy_troops = _preload.EnemyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    //const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();
    
    
    _state[STATE_BUILD_BARRACK] = 1;

    // 最远的飞机距离 目标 90km时 发射 第一枚导弹
    // 最远的飞机距离 目标 70km时，发射第二枚导弹
    
    // float max_distance  = 0; 
    // int round = 0;
    // for(const Unit* u : my_troops[WORKER]){
    //     float distance = PointF::L2Sqr(u->GetPointF(),enemy_troops[BASE][0]->GetPointF()); //测量目标与飞机的距离
    //     round = u->GetProperty().round;
    //     if (distance > max_distance)
    //       max_distance = distance;
    // }

    // if(max_distance > 81.0f){ 
    //     _state[STATE_BUILD_BARRACK] = 1;
    // }else if(max_distance< 81.0f && round > 1){
    //     _state[STATE_ATTACK] = 1;
    // }else if(max_distance < 49.0f && round > 0){
    //     _state[STATE_ATTACK] = 1;
    // }else{
    //     _state[STATE_BUILD_BARRACK] = 1;
    // }
    _state[STATE_BUILD_BARRACK] =1 ;
    // if (my_troops[WORKER].size() < 3 && _preload.Affordable(WORKER)) {
    //     _state[STATE_BUILD_WORKER] = 1;
    // }

    // if (my_troops[WORKER].size() >= 3 && my_troops[BARRACKS].size() + cnt_under_construction[BARRACKS] < 1 && _preload.Affordable(BARRACKS)) {
    //     _state[STATE_BUILD_BARRACK] = 1;
    // }

    // if (my_troops[BARRACKS].size() >= 1 && _preload.Affordable(MELEE_ATTACKER)) {
    //     _state[STATE_BUILD_MELEE_TROOP] = 1;
    // }

    // if (my_troops[MELEE_ATTACKER].size() >= 5 && ! enemy_troops[BASE].empty()) {
    //     _state[STATE_ATTACK] = 1;
    // }
    // if (! enemy_troops_in_range.empty() || ! enemy_attacking_economy.empty()) {
    //     _state[STATE_ATTACK_IN_RANGE] = 1;
    //     _state[STATE_DEFEND] = 1;
    // }
    
    return true;
}

bool MCRuleActor::GetActHitAndRunState(vector<int>* state) {
    vector<int> &_state = *state;

    const auto& my_troops = _preload.MyTroops();
    const auto& cnt_under_construction = _preload.CntUnderConstruction();
    const auto& enemy_troops = _preload.EnemyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();

    if (my_troops[WORKER].size() < 3 && _preload.Affordable(WORKER)) {
        _state[STATE_BUILD_WORKER] = 1;
    }
    if (my_troops[WORKER].size() >= 3 && my_troops[BARRACKS].size() + cnt_under_construction[BARRACKS] < 1 && _preload.Affordable(BARRACKS)) {
        _state[STATE_BUILD_BARRACK] = 1;
    }
    if (my_troops[BARRACKS].size() >= 1 && _preload.Affordable(RANGE_ATTACKER)) {
        _state[STATE_BUILD_RANGE_TROOP] = 1;
    }
    int range_troop_size = my_troops[RANGE_ATTACKER].size();
    if (range_troop_size >= 2) {
        if (enemy_troops[MELEE_ATTACKER].empty() && enemy_troops[RANGE_ATTACKER].empty()
          && enemy_troops[WORKER].empty()) {
            _state[STATE_ATTACK] = 1;
        } else {
            _state[STATE_HIT_AND_RUN] = 1;
        }
    }
    if (! enemy_troops_in_range.empty() || ! enemy_attacking_economy.empty()) {
        _state[STATE_DEFEND] = 1;
    }
    return true;
}

bool MCRuleActor::ActWithMap(const GameEnv &env, const vector<vector<vector<int>>>& action_map, string *state_string, AssignedCmds *assigned_cmds) {
    assigned_cmds->clear();
    *state_string = "";

    vector<vector<RegionHist>> hist(action_map.size());
    for (size_t i = 0; i < hist.size(); ++i) {
        hist[i].resize(action_map[i].size());
    }

    const int x_size = env.GetMap().GetXSize();
    const int y_size = env.GetMap().GetYSize();
    const int rx = action_map.size();
    const int ry = action_map[0].size();

    // Then loop over all my troops to run.
    const auto& all_my_troops = _preload.AllMyTroops();
    for (const Unit *u : all_my_troops) {
        // Get the bin id.
        const PointF& p = u->GetPointF();
        int x = static_cast<int>(std::round(p.x / x_size * rx));
        int y = static_cast<int>(std::round(p.y / y_size * ry));
        // [REGION_MAX_RANGE_X][REGION_MAX_RANGE_Y][REGION_RANGE_CHANNEL]
        act_per_unit(env, u, &action_map[x][y][0], &hist[x][y], state_string, assigned_cmds);
    }

    return true;
}
