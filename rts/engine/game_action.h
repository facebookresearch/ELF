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
    }

    map<UnitId, CmdBPtr> &cmds() { return _cmds; }

    void AddComment(const std::string &comment) {
        if (! comment.empty()) _comments.push_back(comment);
    }

    virtual bool Send(const GameEnv &env, CmdReceiver &receiver) {
        // Finally send these commands.
        for (auto it = _cmds.begin(); it != _cmds.end(); ++it) {
            const Unit *u = env.GetUnit(it->first);
            if (u == nullptr) continue;

            // Cannot give command to other units.
            if (u->GetPlayerId() != _player_id) continue;
            if (! env.GetGameDef().unit(u->GetUnitType()).CmdAllowed(it->second->type())) continue;
            if (env.GetGameDef().IsUnitTypeBuilding(u->GetUnitType()) && receiver.GetUnitDurativeCmd(u->GetId()) != nullptr) {
                std::cout << u->GetUnitType() << std::endl;
                continue;
            }
            it->second->set_id(it->first);

            // Note that after this command, it->second is not usable.
            receiver.SendCmd(std::move(it->second));
        }

        string prompt = "[" + std::to_string(_player_id) + ":" + _name + "] ";
        for (const auto &c : _comments) {
            receiver.SendCmd(CmdBPtr(new CmdComment(INVALID, prompt + c)));
        }

        // UI Cmds are handled separately (In RTSStateExtends::Forward).
        return true;
    }

protected:
    PlayerId _player_id = -1;
    string _name;
    vector<string> _comments;

    map<UnitId, CmdBPtr> _cmds;
};
