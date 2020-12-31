/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#ifndef _BULLET_H_
#define _BULLET_H_

#include "common.h"
#include "unit.h"

// The class is used for flying bullets for long-range attacker and other visual effects,
// E.g., visualization showing a unit is casting a spell (and can be interrupted before it is finished).
// This is useful from the training perspective, since human player picks up these hints and act accordingly.
// The bullets are not included in the locality table (for efficiency for now) and thus not clickable by a mouse event.
class Bullet {
private:
    // Location of the bullet.
    PointF _p;

    // The unit that creates this unit. Bullet does not have its own ids.  发出子弹的单位ID
    UnitId _id_from;  

    // Internal state. E.g., some bullets might explode when hitting the target. Different stage of explosion
    // is carried on by _state, which can be used in rendering.
    BulletState _state;

    // The attack it carries. 攻击力
    int _att;

    // The target id. If INVALID, check target_p
    UnitId _target_id;
    PointF _target_p;

    // The speed it flies. 速度
    float _speed;

public:
    Bullet() : _id_from(INVALID), _state(BULLET_READY), _att(0), _target_id(INVALID), _speed(0.0) {
    }

    Bullet(const PointF &p, const UnitId &id_from, int att, float speed)
        : _p(p), _id_from(id_from), _state(BULLET_READY), _att(att), _target_id(INVALID), _speed(speed) {
    }

    void SetTargetUnitId(const UnitId &id) { _target_id = id; }
    void SetTarget(const PointF &p) { _target_p = p; }

    UnitId GetIdFrom() const { return _id_from; }
    const PointF &GetPointF() const { return _p; }
    BulletState GetState() const { return _state; }

    // Get the visualization command.
    string Draw() const;

    // Unlike Unit, we don't do Act then PerformAct since collision check is not needed. 不需要碰撞检测
    // The bullet will deliver microcommand (to inflict damage and other special effects, e.g., slow-down/healing).
    CmdBPtr Forward(const RTSMap &m, const Units& units);

    // The bullet is dead and needs to be removed.
    bool IsDead() const { return _state == BULLET_DONE; }

    SERIALIZER(Bullet, _p, _speed, _att, _id_from, _target_id, _target_p, _state);
};

typedef vector<Bullet> Bullets;

#endif
