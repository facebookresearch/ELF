/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "raw2cmd.h"
#include "engine/cmd_specific.gen.h"

/////////// RawToCmd /////////////////
//
CmdInput move_event(const Unit &u, char /*hotkey*/, const PointF& p, const UnitId &target_id, const GameEnv&) {
    // Don't need to check hotkey since there is only one type of action.
    if (target_id == INVALID && ! p.IsInvalid()) {
        // cout << "In move command [" << hotkey << "] @" << p << " target: " << target_id << endl;
        return CmdInput(CmdInput::CI_MOVE, u.GetId(), p, target_id);
    }
    else return CmdInput();
}

CmdInput attack_event(const Unit &u, char /*hotkey*/, const PointF& p, const UnitId &target_id, const GameEnv&) {
    // Don't need to check hotkey since there is only one type of action.
    //cout << "In attack command ["  << "] @" << p << " target: " << target_id << endl;
    return CmdInput(CmdInput::CI_ATTACK, u.GetId(), p, target_id);
}



CmdInput gather_event(const Unit &u, char /*hotkey*/, const PointF& p, const UnitId &target_id, const GameEnv& env) {
    // Don't need to check hotkey since there is only one type of action.
    // cout << "In gather command [" << hotkey << "] @" << p << " target: " << target_id << endl;
    UnitId base = env.FindClosestBase(u.GetPlayerId());
    return CmdInput(CmdInput::CI_GATHER, u.GetId(), p, target_id, base);
}

CmdInput build_event(const Unit &u, char hotkey, const PointF& p, const UnitId& /*target_id*/, const GameEnv&) {
    // Send the build command.
    // cout << "In build command [" << hotkey << "] @" << p << " target: " << target_id << endl;
    UnitType t = u.GetUnitType();

    UnitType build_type;
    PointF build_p;

    // For workers: c : base, b: barracks (for workers)
    // For base: s : worker,
    // For building: m : melee attacker, r: range attacker
    // [TODO]: Make it more flexible and print corresponding prompts in GUI.
    switch(t) {
        case WORKER:
            if (p.IsInvalid()) return CmdInput();
            // Set the location.
            build_p = p;
            if (hotkey == 'c') build_type = BASE;
            else if (hotkey == 'b') build_type = BARRACKS;
            else return CmdInput();
            break;
        case BASE:
            build_p.SetInvalid();
            if (hotkey == 's') build_type = WORKER;
            else return CmdInput();
            break;
        case BARRACKS:
            build_p.SetInvalid();
            if (hotkey == 'm') build_type = MELEE_ATTACKER;
            else if (hotkey == 'r') build_type = RANGE_ATTACKER;
            else return CmdInput();
            break;
        default:
            return CmdInput();
    }

    return CmdInput(CmdInput::CI_BUILD, u.GetId(), build_p, INVALID, INVALID, build_type);
}

void RawToCmd::add_hotkey(const string& s, EventResp f) {
    for (size_t i = 0; i < s.size(); ++i) {
        _hotkey_maps.insert(make_pair(s[i], f));
    }
}

void RawToCmd::setup_hotkeys() {
    add_hotkey("a", attack_event);
    add_hotkey("~", move_event);
    add_hotkey("t", gather_event);
    add_hotkey("cbsmr", build_event);
}

RawMsgStatus RawToCmd::Process(Tick tick, const GameEnv &env, const string&s, vector<CmdBPtr> *cmds, vector<UICmd> *ui_cmds) {
    // Raw command:
    //   t 'L' i j: left click at (i, j)
    //   t 'R' i j: right clock at (i, j)
    //   t 'B' x0 y0 x1 y1: bounding box at (x0, y0, x1, y1)
    //   t 'S' percent: slide bar to percentage
    //   t 'F'        : faster
    //   t 'W'        : slower
    //   t 'C'        : cycle through players.
    //   t lowercase : keyboard click.
    // t is tick.
    if (s.empty()) return PROCESSED;
    assert(cmds != nullptr);
    assert(ui_cmds != nullptr);

    Tick t;
    char c;
    float percent;
    PointF p, p2;
    set<UnitId> selected;

    cout << "Cmd: " << s << endl;

    const RTSMap& m = env.GetMap();

    istringstream ii(s);
    string cc;
    ii >> t >> cc;
    if (cc.size() != 1) return PROCESSED;
    c = cc[0];
    switch(c) {
        case 'L':
        case 'R':
            ii >> p;
            if (! m.IsIn(p)) return FAILED;
            {
            UnitId closest_id = m.GetClosestUnitId(p, 1.5);
            if (closest_id != INVALID) selected.insert(closest_id);
            }
            break;
        case 'B':
            ii >> p >> p2;
            if (! m.IsIn(p) || ! m.IsIn(p2)) return FAILED;
            // Reorder the four corners.
            if (p.x > p2.x) swap(p.x, p2.x);
            if (p.y > p2.y) swap(p.y, p2.y);
            selected = m.GetUnitIdInRegion(p, p2);
            break;
        case 'F':
            ui_cmds->push_back(UICmd::GetUIFaster());
            return PROCESSED;
        case 'W':
            ui_cmds->push_back(UICmd::GetUISlower());
            return PROCESSED;
        case 'C':
            ui_cmds->push_back(UICmd::GetUICyclePlayer());
            return PROCESSED;
        case 'S':
            ii >> percent;
            // cout << "Get slider bar notification " << percent << endl;
            ui_cmds->push_back(UICmd::GetUISlideBar(percent));
            return PROCESSED;
        case 'P':
            ui_cmds->push_back(UICmd::GetToggleGamePause());
            return PROCESSED;
        default:
            break;
    }

    if (! is_mouse_motion(c)) _last_key = c;

     //cout << "#Hotkey " << _hotkey_maps.size() << "  player_id = " << _player_id << " _last_key = " << _last_key
     //    << " #selected = " << selected.size() << " #prev-selected: " << _sel_unit_ids.size() << endl;

    // Rules:
    //   1. we cannot control other player's units.
    //   2. we cannot have friendly fire (enforced in the callback function)
    bool cmd_success = false;
    //if(! _sel_unit_ids.empty() && selected.size() >1 && _last_key == 'a')
    if (! _sel_unit_ids.empty() && selected.size() <= 1) {  
        UnitId id = (selected.empty() ? INVALID : *selected.begin());
        auto it_key = _hotkey_maps.find(_last_key);
        if (it_key != _hotkey_maps.end()) {
            EventResp f = it_key->second;
            for (auto it = _sel_unit_ids.begin(); it != _sel_unit_ids.end(); ++it) {
                // cout << "Deal with unit" << *it << endl << flush;
                if (Player::ExtractPlayerId(*it) != _player_id) continue;

                // Only command our unit.
                const Unit *u = env.GetUnit(*it);

                // u has been deleted (e.g., killed)
                // We won't delete it in our selection, since the selection will naturally update.
                if (u == nullptr) continue;

                CmdBPtr cmd = f(*u, _last_key, p, id, env).GetCmd();
                if (! cmd.get() || ! env.GetGameDef().unit(u->GetUnitType()).CmdAllowed(cmd->type())) continue;

                // Command successful.
                cmds->emplace_back(std::move(cmd));
                cmd_success = true;
            }
        }
     }

    

    // Command not issued. if it is a mouse event, simply reselect the unit (or deselect)
    if (! cmd_success && is_mouse_motion(c)) select_new_units(selected);
    if (cmd_success) _last_key = '~';

    if (t > tick) return EXCEED_TICK;
    return PROCESSED;
}
