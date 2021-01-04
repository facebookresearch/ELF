/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "bullet.h"
#include "cmd_specific.gen.h"

//static constexpr float kDistBullet = 0.3;  // 子弹的体积？
static constexpr float kDistBullet = 0.003;  // 子弹的体积？

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
            // The target is destroyed, destroy itself. 目标已经被摧毁，销毁子弹
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

    if (dist_sqr < kDistBullet * kDistBullet) {  // 如果子弹击中目标
        // Hit the target.
        // cout<<"dist_sqr: "<<dist_sqr<<" 碰撞距离: "<<kDistBullet * kDistBullet<<endl;
        // cout<<"bullet_p: "<<_p<<"  目标位置: "<<target<<endl;
        _state = BULLET_EXPLODE1;
        if (_target_id != INVALID) {
            return CmdBPtr(new CmdMeleeAttack(_id_from, _target_id, _att)); // 造成一次攻击
        }
    } else {  // 子弹飞向目标
        // Fly
        // [TODO]: Randomize the flying procedure (e.g., curvy tracking).
        PointF diff;  // 从 _p 指向 target 的方向

        // Here it has to be true, otherwise there is something wrong.
        if (! PointF::Diff(target, _p, &diff)) {
            cout << "Bullet::Forward, target or _p is invalid! Target: " << target << " _p:" << _p << endl;
            return CmdBPtr();
        }
        
        diff.Trunc(_speed); // 移动的距离
        //std::cout<<"dist: "<<diff.x*diff.x + diff.y*diff.y<<std::endl;
        _p += diff;  // 更新子弹位置
    }
    return CmdBPtr();
}
