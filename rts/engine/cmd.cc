/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "cmd.h"
#include "game_env.h"
#include "cmd_receiver.h"
#include "common.h"
#include "gamedef.h"
#include "cmd.gen.h"

SERIALIZER_ANCHOR_INIT(CmdBase);
SERIALIZER_ANCHOR_INIT(CmdImmediate);
SERIALIZER_ANCHOR_INIT(CmdDurative);

std::map<std::string, int> CmdTypeLookup::_name2idx;
std::map<int, std::string> CmdTypeLookup::_idx2name;
std::string CmdTypeLookup::_null;
std::mutex CmdTypeLookup::_mutex;

/*
static float trunc(float v, float b) {
    return std::max(std::min(v, b), -b);
}
*/
#define MT_OK 0
#define MT_TARGET_INVALID 1
#define MT_ARRIVED 2
#define MT_CANNOT_MOVE 3

static int move_toward(const RTSMap& m, float speed, const UnitId& id,
        const PointF& curr, const PointF& target, PointF *move) {
    // Given curr location, move towards the target.
    PointF diff;

    if (! PointF::Diff(target, curr, &diff)) return MT_TARGET_INVALID;
    if (std::abs(diff.x) < kDistEps && std::abs(diff.y) < kDistEps) return MT_ARRIVED;
    //bool movable = false;
    while (true) {
        
        diff.Trunc(speed);
        PointF next_p(curr);
        next_p += diff;

        bool movable = m.CanPass(next_p, id);
        // cout << "MoveToward [" << id << "]: Try straight: " << next_p << " movable: " << movable << endl;
        if (! movable) {
            next_p = curr;
            next_p += diff.CCW90();
            movable = m.CanPass(next_p, id);
            // cout << "MoveToward [" << id << "]: Try CCW: " << next_p << " movable: " << movable << endl;
        }
        if (! movable) {
            next_p = curr;
            next_p += diff.CW90();
            movable = m.CanPass(next_p, id);
            // cout << "MoveToward [" << id << "]: Try CW: " << next_p << " movable: " << movable << endl;
        }
        

        // If we still cannot move, then we reduce the speed.
        if (movable) {
            *move = next_p;
            return MT_OK;
        }
        else return MT_CANNOT_MOVE;

        /*
        speed /= 1.2;
        // If the move speed is too slow, then we skip.
        if (speed < 0.005) break;
        */
    }
}

float micro_move(Tick tick, const Unit& u, const GameEnv &env, const PointF& target, CmdReceiver *receiver) {
    const RTSMap &m = env.GetMap();
    const PointF &curr = u.GetPointF();  // 当前位置
    const Player &player = env.GetPlayer(u.GetPlayerId());

    //cout << "Micro_move: Current: " << curr << " Target: " << target << endl;
    float dist_sqr = PointF::L2Sqr(target, curr); // 距离 （平方）
    const UnitProperty &property = u.GetProperty();

    static const int kMaxPlanningIteration = 1000;

    // Move to a target. Ideally we should do path-finding, for now just follow L1 distance.
    if ( property.CD(CD_MOVE).Passed(tick) ) {
        PointF move;

        float dist_sqr = PointF::L2Sqr(curr, target);
        PointF waypoint = target; // 找到下一步的格子 waypoint
        bool planning_success = false;
       
        if (dist_sqr > 1.0) {
            // Do path planning.
            Coord first_block;
            float est_dist;
            planning_success = player.PathPlanning(tick, u.GetId(), curr, target,
                kMaxPlanningIteration, receiver->GetPathPlanningVerbose(), &first_block, &est_dist);
            if (planning_success && first_block.x >= 0 && first_block.y >= 0) {
                waypoint.x = first_block.x;
                waypoint.y = first_block.y;
            }
        }
        //cout << "micro_move: (" << curr << ") -> (" << waypoint << ") planning: " << planning_success << endl;
        int ret = move_toward(m, property._speed, u.GetId(), curr, waypoint, &move);
        if (ret == MT_OK) {
            // Set actual move.
            receiver->SendCmd(CmdIPtr(new CmdTacticalMove(u.GetId(), move)));
        } else if (ret == MT_CANNOT_MOVE) {
            // Unable to move. This is usually due to PathPlanning issues.
            // Too many such commands will leads to early termination of game.
            // [TODO]: Make PathPlanning better.
            receiver->GetGameStats().RecordFailedMove(tick, 1.0);
        }
    }
    
    return dist_sqr;
}

float circle_move(Tick tick, const Unit& u, const GameEnv &env, const PointF& target, CmdReceiver *receiver,float angle) {
    const RTSMap &m = env.GetMap();
    const PointF &curr = u.GetPointF();  // 当前位置
    const Player &player = env.GetPlayer(u.GetPlayerId());
    
    //cout << "Micro_move: Current: " << curr << " Target: " << target << endl;
    float dist_sqr = PointF::L2Sqr(target, curr); // 距离 （平方）
    const UnitProperty &property = u.GetProperty();

    static const int kMaxPlanningIteration = 1000;

    // Move to a target. Ideally we should do path-finding, for now just follow L1 distance.
    if ( property.CD(CD_MOVE).Passed(tick) ) {
        PointF move;

        float dist_sqr = PointF::L2Sqr(curr, target);
        PointF waypoint = target; // 找到下一步的格子 waypoint
        bool planning_success = false;
       
        if (dist_sqr > 1.0) {
            // Do path planning.
            Coord first_block;
            float est_dist;
            planning_success = player.PathPlanning(tick, u.GetId(), curr, target,
                kMaxPlanningIteration, receiver->GetPathPlanningVerbose(), &first_block, &est_dist);
            if (planning_success && first_block.x >= 0 && first_block.y >= 0) {
                waypoint.x = first_block.x;
                waypoint.y = first_block.y;
            }
        }
        //cout << "micro_move: (" << curr << ") -> (" << waypoint << ") planning: " << planning_success << endl;
        int ret = move_toward(m, property._speed, u.GetId(), curr, waypoint, &move);
        if (ret == MT_OK) {
            // Set actual move.
            receiver->SendCmd(CmdIPtr(new CmdTacticalMove(u.GetId(), move)));
        } else if (ret == MT_CANNOT_MOVE) {
            // Unable to move. This is usually due to PathPlanning issues.
            // Too many such commands will leads to early termination of game.
            // [TODO]: Make PathPlanning better.
            receiver->GetGameStats().RecordFailedMove(tick, 1.0);
        }
    }
    
    return dist_sqr;
}





bool CmdDurative::Run(const GameEnv &env, CmdReceiver *receiver) {
    // If we run this command for the first time, register it in the receiver.
    if (_start_tick == _tick) receiver->StartDurativeCmd(this);

    // Run the script.
    bool ret = run(env, receiver);

    _tick = receiver->GetNextTick();
    return ret;
}

// ============= Commands ==============
// ----- Move
bool CmdTacticalMove::run(GameEnv *env, CmdReceiver*) {
    //std::cout<<"===========CmdTacticalMove========="<<std::endl;
    Unit *u = env->GetUnit(_id);
    if (u == nullptr) return false;
    RTSMap &m = env->GetMap();

    // Move a unit.
    if (m.MoveUnit(_id, _p)) {
        u->SetPointF(_p);
        u->GetProperty().CD(CD_MOVE).Start(_tick);
        return true;
    }
    else return false;
}

bool CmdCDStart::run(GameEnv *env, CmdReceiver*) {
    Unit *u = env->GetUnit(_id);
    if (u == nullptr) return false;
    u->GetProperty().CD(_cd_type).Start(_tick);
    return true;
}

bool CmdEmitBullet::run(GameEnv *env, CmdReceiver*) {
    if (_id == INVALID) return false;
    Unit *u = env->GetUnit(_id);
    if (u == nullptr) return false;

    // cout << "Bullet: " << micro_cmd.PrintInfo() << endl;
    Bullet b(_p, _id, _att, _speed); // 生成一颗子弹
    b.SetTargetUnitId(_target);   // 设置子弹的目标
    env->AddBullet(b);  // 环境中添加这颗子弹
    return true;
}

bool CmdCreate::run(GameEnv *env, CmdReceiver*) {
    // Create a unit at a location
    //std::cout<<"CmdCreate: "<<PrintInfo()<<std::endl;
    UnitId u_id ;
    if (! env->AddUnit(_tick, _build_type, _p, _player_id,u_id)) {
        // If failed, money back!
        //std::cout<<"failed"<<std::endl;
        env->GetPlayer(_player_id).ChangeResource(_resource_used);
        return false;
    }
   //std::cout<<"CmdCreate u_id: "<< u_id<<std::endl;
    return u_id;
}

bool CmdRemove::run(GameEnv *env, CmdReceiver* receiver) {
    if (env->RemoveUnit(_id)) {
        receiver->FinishDurativeCmd(_id);
        return true;
    } else return false;
}

bool CmdLoadMap::run(GameEnv *env, CmdReceiver*) {
    serializer::loader loader(false);
    if (! loader.read_from_file(_s)) return false;
    loader >> env->GetMap();
    return true;
}

bool CmdSaveMap::run(GameEnv *env, CmdReceiver*) {
    serializer::saver saver(false);
    saver << env->GetMap();
    if (! saver.write_to_file(_s)) return false;
    return true;
}

bool CmdRandomSeed::run(GameEnv *env, CmdReceiver*) {
    env->SetSeed(_seed);
    return true;
}

bool CmdComment::run(GameEnv*, CmdReceiver*) {
    // COMMENT command does not do anything, but leave a record.
    return true;
}
