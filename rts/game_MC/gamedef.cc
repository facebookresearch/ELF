/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "engine/gamedef.h"
#include "engine/game_env.h"
#include "engine/rule_actor.h"

#include "engine/cmd.gen.h"
#include "engine/cmd_specific.gen.h"
#include "cmd_specific.gen.h"
#include "engine/ai.h"
#include "rule_ai.h"

int GameDef::GetNumUnitType() {
    return NUM_MINIRTS_UNITTYPE;
}

int GameDef::GetNumAction() {
    return NUM_AISTATE;
}

bool GameDef::IsUnitTypeBuilding(UnitType t) const{
    // return (t == BASE) || (t == RESOURCE) || (t == BARRACKS);
    return (t == BASE) || (t == BARRACKS);
}

bool GameDef::HasBase() const{ return true; }

bool GameDef::CheckAddUnit(RTSMap *_map, UnitType, const PointF& p) const{
    return _map->CanPass(p, INVALID);
}

void GameDef::GlobalInit() {
    reg_engine();   //
    reg_engine_specific();
    reg_minirts_specific();

    // InitAI.
    AIFactory<AI>::RegisterAI("simple", [](const std::string &spec) {
        (void)spec;
        AIOptions ai_options;
        return new SimpleAI(ai_options);
    });

    AIFactory<AI>::RegisterAI("hit_and_run", [](const std::string &spec) {
        (void)spec;
        AIOptions ai_options;
        return new HitAndRunAI(ai_options);
    });
}

void GameDef::Init() {
    _units.assign(GetNumUnitType(), UnitTemplate());
    /**
     * int cost, int hp, int defense, float speed, int att, int att_r, int vis_r,
        const vector<int> &cds, const vector<CmdType> &l, UnitAttr attr**/

    // _units[RESOURCE] = _C(0, 1000, 1000, 0, 0, 0, 0, vector<int>{0, 0, 0, 0}, vector<CmdType>{}, ATTR_INVULNERABLE);
    // _units[WORKER] = _C(50, 50, 0, 0.1, 2, 1, 3, vector<int>{0, 10, 40, 40}, vector<CmdType>{MOVE, ATTACK, BUILD, GATHER});
    // _units[MELEE_ATTACKER] = _C(100, 100, 1, 0.1, 15, 1, 3, vector<int>{0, 15, 0, 0}, vector<CmdType>{MOVE, ATTACK});
    // _units[RANGE_ATTACKER] = _C(100, 50, 0, 0.2, 10, 5, 5, vector<int>{0, 10, 0, 0}, vector<CmdType>{MOVE, ATTACK});
    // _units[BARRACKS] = _C(200, 200, 1, 0.0, 0, 0, 5, vector<int>{0, 0, 0, 50}, vector<CmdType>{BUILD});
    // _units[BASE] = _C(500, 500, 2, 0.0, 0, 0, 5, {0, 0, 0, 50}, vector<CmdType>{BUILD});

 /**   
 *  定义单位属性
 *  cost   造价(不需要)
 *  hp     血量
 *  defence防御力 
 *  speed  速度
 *  att    攻击力
 *  att_r  攻击距离
 *  vis_r  可视距离
 *  round  载弹量   -1 -- 无限弹药/不适用
 *  isReturn 飞行状态 true -- 返航  false -- 飞向目标
 *  towards 朝向，计算FOW
 *  flight_state // 飞行状态 IDLE -- 空闲  MOVE -- 飞向目标  RETURN -- 返航  0.03
 * *                     cost hp    def  sp att att_r vis_r round
 * *                 cd   move attack gather build
 * */
     
    _units[RESOURCE] = _C(0, 1000, 1000, 0, 0, 0, 0, -1, INVALID_FLIGHTSTATE,vector<int>{0, 0, 0, 0}, vector<CmdType>{}, ATTR_INVULNERABLE);
    // 飞机 移动 发射导弹
    _units[WORKER] = _C(100, 100, 0, 0.2, 2, 12.0f, 0,2,FLIGHT_IDLE,vector<int>{0, 40, 0, 0}, vector<CmdType>{MOVE, ATTACK,CIRCLE_MOVE});
    // 炮塔 攻击(只能攻击锁定的目标)
    _units[MELEE_ATTACKER] = _C(50, 100, 0, 0.1, 100, 20.0f, 0, 8,INVALID_FLIGHTSTATE,vector<int>{0, 5, 0, 0}, vector<CmdType>{ATTACK});
    // 雷达 索敌
    _units[RANGE_ATTACKER] = _C(100, 100, 0, 0.2, 0, 30.0f, 30, -1,INVALID_FLIGHTSTATE,vector<int>{0, 0, 0, 0}, vector<CmdType>{ATTACK});
    // 导弹 移动 攻击(攻击范围很小)
    _units[BARRACKS] = _C(0, 10, 0, 0.03, 100, 0.1, 0, -1,INVALID_FLIGHTSTATE,vector<int>{0, 40, 0, 50}, vector<CmdType>{ATTACK});
    // 保护目标
    _units[BASE] = _C(500, 200, 0, 0.0, 0, 0, 0, -1, INVALID_FLIGHTSTATE, vector<int>{0, 0, 0, 50},vector<CmdType>{BUILD});
}

vector<pair<CmdBPtr, int> > GameDef::GetInitCmds(const RTSGameOptions&) const{
      vector<pair<CmdBPtr, int> > init_cmds;
      init_cmds.push_back(make_pair(CmdBPtr(new CmdGenerateMap(INVALID, 0, 0)), 1));  // 障碍 资源 
      init_cmds.push_back(make_pair(CmdBPtr(new CmdGenerateUnit(INVALID)), 2));   
      return init_cmds;
}

// 通过判断最后一个拥有基地的玩家来确定胜利者
PlayerId GameDef::CheckWinner(const GameEnv& env, bool /*exceeds_max_tick*/) const {
    //首先判断玩家的基地是否还存在
    //判断场上是否还有敌方单位 (加上敌方是否还有剩余飞机的判定)
    return env.CheckWinner(BASE);
   
}

void GameDef::CmdOnDeadUnitImpl(GameEnv* env, CmdReceiver* receiver, UnitId /*_id*/, UnitId _target) const{
    Unit *target = env->GetUnit(_target);
    if (target == nullptr) return;
    receiver->SendCmd(CmdIPtr(new CmdRemove(_target)));
}
