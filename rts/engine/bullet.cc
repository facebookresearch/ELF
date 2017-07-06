/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "bullet.h"
#include "cmd_specific.gen.h"

static constexpr float kDistBullet = 0.3;

string Bullet::Draw() const {
    return make_string("u", _p, _state);
}

// Unlike Unit, we don't do Act then PerformAct since collision check is not needed.
CmdBPtr Bullet::Forward(const RTSMap&, const Units& units) {
    // First check whether the attacker is dead, if so, remove _id_from to avoid issues.
    auto self_it = units.find(_id_from);
    if (self_it == units.end()) _id_from = INVALID;

    // If it already exploded, the state changes until it goes to DONE.
    if (_state == BULLET_EXPLODE1) {
        _state = BULLET_EXPLODE2;
        return CmdBPtr();
    } else if (_state == BULLET_EXPLODE2) {
        _state = BULLET_EXPLODE3;
        return CmdBPtr();
    } else if (_state == BULLET_EXPLODE3) {
        _state = BULLET_DONE;
        return CmdBPtr();
    }

    // Move forward with the speed provided.
    // Hit the target
    // Change its state until it is done.
    PointF target;
    if (_target_id != INVALID) {
        auto it = units.find(_target_id);
        if (it == units.end()) {
            // The target is destroyed, destroy itself.
            _state = BULLET_DONE;
            return CmdBPtr();
        }
        target = it->second->GetPointF();
    } else {
        if (_target_p.IsInvalid()) {
            _state = BULLET_DONE;
            return CmdBPtr();
        }
        target = _target_p;
    }

    float dist_sqr = PointF::L2Sqr(_p, target);

    if (dist_sqr < kDistBullet * kDistBullet) {
        // Hit the target.
        _state = BULLET_EXPLODE1;
        if (_target_id != INVALID) {
            return CmdBPtr(new CmdMeleeAttack(_id_from, _target_id, _att));
        }
    } else {
        // Fly
        // [TODO]: Randomize the flying procedure (e.g., curvy tracking).
        PointF diff;

        // Here it has to be true, otherwise there is something wrong.
        if (! PointF::Diff(target, _p, &diff)) {
            cout << "Bullet::Forward, target or _p is invalid! Target: " << target << " _p:" << _p << endl;
            return CmdBPtr();
        }

        diff.Trunc(_speed);
        _p += diff;
    }
    return CmdBPtr();
}
