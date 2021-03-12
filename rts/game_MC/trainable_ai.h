/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include <sstream>
#include "ai.h"
#include "elf/circular_queue.h"


class Preload_Train {
public:
    enum Result { NOT_READY = -1, OK = 0, NO_BASE };

private:
    vector<vector<const Unit*> > _my_troops;  //我方单位
    vector<const Unit*> _enemy_troops_in_range; // 视野内的敌方单位
    vector<const Unit*> _all_my_troops;

    UnitId _base_id;
    PointF _base_loc;

    const Unit *_base = nullptr;
    PlayerId _player_id = INVALID;
    int _num_unit_type = 0;
    Result _result = NOT_READY;
    

    static bool InCmd(const CmdReceiver &receiver, const Unit &u, CmdType cmd) {
        const CmdDurative *c = receiver.GetUnitDurativeCmd(u.GetId());
        return (c != nullptr && c->type() == cmd);
    }

    void collect_stats(const GameEnv &env, int player_id);

public:
    Preload_Train() { }

    void GatherInfo(const GameEnv &env, int player_id);
    Result GetResult() const { return _result; }
    
    
   

    
    //CmdBPtr GetAccackEnemyUnitCmd(UnitId t_id) const { return _A(t_id);}
    
    
    // 随机获取攻击范围内的指定类型目标
    // custom_enum(FlightType, INVALID_FLIGHTTYPE = -1, FLIGHT_NORMAL = 0, FLIGHT_BASE, FLIGHT_TOWER, FLIGHT_FAKE, NUM_FLIGHT);  // 飞机种类
    
    const PointF &BaseLoc() const { return _base_loc; }
    const Unit *Base() const { return _base; }
    

    const vector<vector<const Unit*> > &MyTroops() const { return _my_troops; }
    const vector<const Unit*> &EnemyTroopsInRange() const { return _enemy_troops_in_range; }
    const vector<const Unit*> &AllMyTroops() const { return _all_my_troops; }
    

};

class TrainedAI : public AIWithComm {
public:
    using State = AIWithComm::State;
    using Action = AIWithComm::Action;
    using Data = typename AIWithComm::Data;

    TrainedAI() : _respect_fow(true), _recent_states(1) { }
    TrainedAI(const AIOptions &opt)
        : AIWithComm(opt.name), _respect_fow(opt.fow), _recent_states(opt.num_frames_in_state) {
      for (auto &v : _recent_states.v()) v.clear();
    }

    bool GameEnd() override;

protected:
     // Preload _preload;
    const bool _respect_fow;
    Preload_Train _preload;

    // History to send.
    CircularQueue<std::vector<float>> _recent_states;

    void compute_state(std::vector<float> *state);
    // Feature extraction.
    void extract(const State &, Data *data) override;
    bool handle_response(const State &, const Data &data, RTSMCAction *a) override;
    bool GatherInfo(const GameEnv &env,PlayerId _player_id);  // 收集执行命令需要的信息
};

