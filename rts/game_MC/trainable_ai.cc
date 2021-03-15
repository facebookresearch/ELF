/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "trainable_ai.h"

#include "engine/game_env.h"
#include "engine/unit.h"
#include "engine/cmd_interface.h"
#include "python_options.h"
#include "state_feature.h"




/*
static inline int sampling(const std::vector<float> &v, std::mt19937 *gen) {
    std::vector<float> accu(v.size() + 1);
    std::uniform_real_distribution<> dis(0, 1);
    float rd = dis(*gen);

    accu[0] = 0;
    for (size_t i = 1; i < accu.size(); i++) {
        accu[i] = v[i - 1] + accu[i - 1];
        if (rd < accu[i]) {
            return i - 1;
        }
    }

    return v.size() - 1;
}
*/

// class Preload {
// public:
//     enum Result { NOT_READY = -1, OK = 0, NO_BASE, NO_RESOURCE };

// private:
//     vector<vector<const Unit*> > _my_troops;  //我方单位
//     vector<vector<const Unit*> > _enemy_troops; // 敌方单位
//     vector<const Unit*> _enemy_troops_in_range;

    
//     PlayerId _player_id = INVALID;
//     int _num_unit_type = 0;

//     void collect_stats(const GameEnv &env, int player_id, const CmdReceiver &receiver);

// public:
//     Preload() { }

//     void GatherInfo(const GameEnv &env, int player_id, const CmdReceiver &receiver);
    

   


//     const vector<vector<const Unit*> > &MyTroops() const { return _my_troops; }
//     const vector<vector<const Unit*> > &EnemyTroops() const { return _enemy_troops; }
//     const vector<const Unit*> &EnemyTroopsInRange() const { return _enemy_troops_in_range; }
// };

bool TrainedAI::GameEnd() {
    AIWithComm::GameEnd();
    for (auto &v : _recent_states.v()) {
        v.clear();
    }
    return true;
}

void TrainedAI::extract(const State &s, Data *data) {
    GameState *game = &data->newest();

    MCExtractor::SaveInfo(s, id(), game);
    game->name = name();

    if (_recent_states.maxlen() == 1) {
        //std::cout<<"TrainAble AI Extract"<<std::endl;
        MCExtractor::Extract(s, id(), _respect_fow, &game->s);
        //std::cout << "(1) size_s = " << game->s.size() << std::endl << std::flush;
    } else {
        std::vector<float> &state = _recent_states.GetRoom();
        MCExtractor::Extract(s, id(), _respect_fow, &state);

        const size_t maxlen = _recent_states.maxlen();
        game->s.resize(maxlen * state.size());
        // std::cout << "(" << maxlen << ") size_s = " << game->s.size() << std::endl << std::flush;
        std::fill(game->s.begin(), game->s.end(), 0.0);
        // Then put all states to game->s.
        for (size_t i = 0; i < maxlen; ++i) {
            const auto &curr_s = _recent_states.get_from_push(i);
            if (! curr_s.empty()) {
                assert(curr_s.size() == state.size());
                std::copy(curr_s.begin(), curr_s.end(), &game->s[i * curr_s.size()]);
            }
        }
    }
}

#define ACTION_GLOBAL 0
#define ACTION_UNIT_CMD 1
#define ACTION_REGIONAL 2

bool TrainedAI::handle_response(const State &s, const Data &data, RTSMCAction *a) {
    //printf(" Handle Response\n");
    a->Init(id(), name());

    // if (_receiver == nullptr) return false;
    const auto &env = s.env();

    // Get the current action from the queue.
    const auto &m = env.GetMap();
    const GameState& gs = data.newest();

    bool gather_ok = GatherInfo(env,a->GetPlayerId());
        if (! gather_ok) {
            //cout<<"gather fail"<<endl;
            return false;
        }
    
    
    // for(int i=0;i<_preload.EnemyTroopsInRange().size();++i){
    //     const Unit* u = _preload.EnemyTroopsInRange()[i];
    //     printf("敌人Id： %d, 距离： %f  \n",u->GetId(), sqrt(PointF::L2Sqr(u->GetPointF(),_preload.MyTroops()[BASE][0]->GetPointF()) )  );
    // }

    switch(gs.action_type) {
        case ACTION_GLOBAL:
            // action
            //
            {
              string comment = "V: " + std::to_string(gs.V) + ", Prob: ";
              for (int i = 0; i < (int) gs.pi.size(); ++i) {
                  if (i > 0) comment += ", ";
                  comment += std::to_string(gs.pi[i]);
              }
              a->AddComment(comment);

              // Also save prev seen.
              a->AddComment("PrevSeenCount: " + std::to_string(env.GetPrevSeenCount(id())));
            }
            a->SetState9(gs.a);
            break;

        case ACTION_UNIT_CMD:
            {
               
                // Use gs.unit_cmds
                //std::vector<CmdInput> unit_cmds(gs.unit_cmds);
                // Use data
                std::vector<CmdInput> unit_cmds(gs.n_max_cmd,CmdInput());
               // std::vector<CmdInput> unit_cmds_1(gs.n_max_cmd,CmdInput());
               
                //std::cout<<"  Action Type "<<gs.action_type <<"  " <<gs.n_max_cmd<<std::endl;
                for (int i = 0; i < gs.n_max_cmd; ++i) {
                   // printf("Handle response ct = %d select %d attack %d\n",gs.ct[i],gs.uloc[i],gs.tloc[i]);
                    int ct = -1;  // Invalid
                    int towerNums = 0;
                    int enemyNums = 0;
                    int tower_id = -1;
                    int target_id = -1;
                    
                    //int _t = rand()%9999+10000;
                    if(gs.ct[i]>0){   //执行攻击命令
                    //    ct = 1;
                       enemyNums = _preload.EnemyTroopsInRange().size();
                       towerNums =  _preload.MyTroops()[MELEE_ATTACKER].size();
                       if(enemyNums>0 && towerNums>0){
                           //printf("enemy Select: %d Total %d towerSelect: %d total %d\n",gs.tloc[i]%enemyNums,enemyNums,gs.uloc[i]%towerNums,towerNums);
                        
                           const Unit* u_tower = _preload.MyTroops()[MELEE_ATTACKER][gs.uloc[i]%towerNums];
                           if(u_tower != nullptr) {
                                tower_id = u_tower->GetId();
                           } 

                           const Unit* u_enemy = _preload.EnemyTroopsInRange()[gs.tloc[i]%enemyNums];
                           if(u_enemy != nullptr) {
                               target_id = u_enemy->GetId();
                           
                           } 

                           unit_cmds[i].Initialize(ct,tower_id,target_id,gs.ct[i]);

                       }
                    }
                    
                   
                   //printf("unitcmd[%d]: %d attack %d with %d rounds\n",i,unit_cmds[i].id,unit_cmds[i].target,unit_cmds[i].round);
                   // unit_cmds.emplace_back(tower,_t,gs.ct[i],ct);
                 
                }
                //std::for_each(unit_cmds.begin(), unit_cmds.end(), [&](CmdInput &ci) { ci.ApplyEnv(env); } );
                
                //printf("SetUnitCmds\n");
                a->SetUnitCmds(unit_cmds);
              
                
            }
            break;

            /*
        case ACTION_REGIONAL:
            {
              if (_receiver->GetUseCmdComment()) {
                string s;
                for (size_t i = 0; i < gs.a_region.size(); ++i) {
                  for (size_t j = 0; j < gs.a_region[0].size(); ++j) {
                    int a = -1;
                    for (size_t k = 0; k < gs.a_region[0][0].size(); ++k) {
                      if (gs.a_region[k][i][j] == 1) {
                        a = k;
                        break;
                      }
                    }
                    s += to_string(a) + ",";
                  }
                  s += "; ";
                }
                SendComment(s);
              }
            }
            // Regional actions.
            return _mc_rule_actor.ActWithMap(env,reply.action_regions, &a->state_string(), &a->cmds());
            */
        default:
            throw std::range_error("action_type not valid! " + to_string(gs.action_type));
    }
    return true;
}


bool TrainedAI::GatherInfo(const GameEnv &env,PlayerId _player_id){
    _preload.GatherInfo(env, _player_id);
     
    auto res = _preload.GetResult();
    if (res == Preload_Train::NO_BASE) return false;
    return true;
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
    _base_id = _base->GetId();
    _base_loc = _base->GetPointF();
    _result = OK;
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
        const Unit *u = it->second.get();
        if (u == nullptr) cout << "Unit cannot be nullptr" << endl << flush;

        if (u->GetPlayerId() == _player_id ) {  // 我方单位
            _my_troops[u->GetUnitType()].push_back(u);
           _all_my_troops.push_back(u);
        } else {  // 敌方单位
            if (player.FilterWithFOW(*u)) {
                if (u->GetUnitType() != RESOURCE) {
                    _enemy_troops_in_range.push_back(u);
                }
            }
        }
    }
}