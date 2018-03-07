#include "utils.h"

#include "engine/cmd.gen.h"
#include "engine/cmd_specific.gen.h"

bool StateProxy::SendCmdCreate(int build_type, const PointF& p, int player_id, int resource_used) {
    return cmd_receiver_->SendCmd(CmdIPtr(new CmdCreate(INVALID, static_cast<UnitType>(build_type),
        p, static_cast<PlayerId>(player_id), resource_used)));
}

bool StateProxy::SendCmdChangePlayerResource(int player_id, int delta) {
    return cmd_receiver_->SendCmd(CmdIPtr(new CmdChangePlayerResource(INVALID, static_cast<PlayerId>(player_id), delta)));
}

