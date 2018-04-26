/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "gamedef.h"

UnitTemplate _C(int cost, int hp, int defense, float speed, int att, int att_r, int vis_r,
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
    for (int i = 0; i < NUM_COOLDOWN; ++i) {
        p._cds[i].Set(cds[i]);
    }

    for (const auto &i : l) {
        res._allowed_cmds.insert(i);
    }

    return res;
};
