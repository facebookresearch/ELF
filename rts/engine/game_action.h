#pragma once
#include <vector>
#include <map>
#include <string>

#include "cmd.h"
#include "cmd.gen.h"
#include "ui_cmd.h"
#include "game_env.h"
#include "cmd_receiver.h"
#include "rule_actor.h"

struct RTSAction {
public:
    void InitState9() {
        _state.resize(NUM_AISTATE);
        std::fill (_state.begin(), _state.end(), 0);
    }

    void SetPlayer(PlayerId id, const string &name) { _player_id = id; _name = name; }
    map<UnitId, CmdBPtr> &cmds() { return _cmds; }
    vector<UICmd> &ui_cmds() { return _ui_cmds; }
    string &state_string() { return _state_string; }
    vector<int> &state() { return _state; }

    void Send(const GameEnv &env, CmdReceiver &receiver) {
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
        // Send UI cmds.
        for (auto &ui_cmd : _ui_cmds) {
            receiver.SendCmd(std::move(ui_cmd));
        }

        auto cmt = "[" + std::to_string(_player_id) + ":" + _name + "] " + _state_string;
        receiver.SendCmd(CmdBPtr(new CmdComment(INVALID, cmt)));
    }

private:
    PlayerId _player_id;
    std::string _name;
    map<UnitId, CmdBPtr> _cmds;
    vector<UICmd> _ui_cmds;

    std::string _state_string;
    vector<int> _state;
};

