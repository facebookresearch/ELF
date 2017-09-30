#pragma once
#include <vector>
#include <map>
#include <string>

#include "cmd.h"
#include "cmd.gen.h"
#include "ui_cmd.h"
#include "game_env.h"
#include "cmd_receiver.h"

class RTSAction {
public:
    void Init(PlayerId id, const string &name) {
        _player_id = id; 
        _name = name;
        _cmds.clear();
        _ui_cmds.clear();
    }

    map<UnitId, CmdBPtr> &cmds() { return _cmds; }
    vector<UICmd> &ui_cmds() { return _ui_cmds; }

    virtual bool Send(const GameEnv &env, CmdReceiver &receiver) { 
        // Finally send these commands.
        for (auto it = _cmds.begin(); it != _cmds.end(); ++it) {
            const Unit *u = env.GetUnit(it->first);
            if (u == nullptr) continue;

            // Cannot give command to other units.
            if (u->GetPlayerId() != _player_id) continue;
            if (! env.GetGameDef().unit(u->GetUnitType()).CmdAllowed(it->second->type())) continue;

            it->second->set_id(it->first);

            // Note that after this command, it->second is not usable.
            receiver.SendCmd(std::move(it->second));
        }

        auto cmt = "[" + std::to_string(_player_id) + ":" + _name + "] " + _state_string;
        receiver.SendCmd(CmdBPtr(new CmdComment(INVALID, cmt)));

        // UI Cmds are handled separately (In RTSStateExtends::Forward).
        return true;
    }

protected:
    PlayerId _player_id = -1;
    std::string _name;
    std::string _state_string;

    map<UnitId, CmdBPtr> _cmds;
    vector<UICmd> _ui_cmds;
};

