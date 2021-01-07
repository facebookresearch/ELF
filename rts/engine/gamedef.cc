/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "gamedef.h"
/**
 *  定义单位属性
 *  cost   造价(不需要)
 *  hp     血量
 *  defence防御力 
 *  speed  速度
 *  att    攻击力
 *  att_r  攻击距离
 *  vis_r  可视距离
 * */
UnitTemplate _C(int cost, int hp, int defense, float speed, int att, float att_r, int vis_r,int round,
        const vector<int> &cds, const vector<CmdType> &l, UnitAttr attr) {

    UnitTemplate res;
    res._build_cost = cost;
    auto &p = res._property;
    p._hp = p._max_hp = hp;
    p._speed = speed;
    p._def = defense;
    p._att = att;
    p._attr = attr;
    p._att_r = att_r;
    p._vis_r = vis_r;
    p.round = round;
    for (int i = 0; i < NUM_COOLDOWN; ++i) {   //设置CD
        p._cds[i].Set(cds[i]);
    }

    for (const auto &i : l) {
        res._allowed_cmds.insert(i);
    }

    return res;
};
