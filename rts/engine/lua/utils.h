#pragma once

#include "engine/map.h"
#include "engine/cmd_receiver.h"

// Hack: Selene doesn't allow to pass more than 1 userdata param, see: https://github.com/jeremyong/Selene/issues/140
class StateProxy {
private:
    RTSMap* map_;
    CmdReceiver* cmd_receiver_;

public:
    StateProxy() = default;
    StateProxy(RTSMap* map, CmdReceiver* cmd_receiver) : map_(map), cmd_receiver_(cmd_receiver) { }
    // RTSMap commands
    int GetXSize() const { return map_->GetXSize(); }
    int GetYSize() const { return map_->GetYSize(); }
    int GetSlotType(int x, int y, int z) { return map_->GetSlotType(x, y, z); }
    // CmdReceiver commands
    bool SendCmdCreate(int build_type, const PointF& p, int player_id, int resource_used = 0);
    bool SendCmdChangePlayerResource(int player_id, int delta);
};


struct AttackRules {
    bool CanAttack(UnitType unit, UnitType target);
};
