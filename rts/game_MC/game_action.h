#pragma once

#include "engine/game_action.h"
#include "mc_rule_actor.h"

class RTSMCAction : public RTSAction {
private:
    enum ActionType { STATE9 = 0, SIMPLE = 1, TOWER_DEFENSE = 2, CMD_INPUT = 3 };

public:
    RTSMCAction() : _type(CMD_INPUT), _action(-1) { }

    void SetState9(int action) {
        _type = STATE9;
        _action = action;
    }

    void SetSimpleAI() { _type = SIMPLE; }
    void SetTowerDefenseAI() { _type = TOWER_DEFENSE; }

    void SetUnitCmds(const std::vector<CmdInput> &unit_cmds) {
        _type = CMD_INPUT;
        _unit_cmds = unit_cmds;
    }

    bool Send(const GameEnv &env, CmdReceiver &receiver);

protected:
    ActionType _type;
    int _action;
    std::vector<CmdInput> _unit_cmds;
};
