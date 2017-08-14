/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _RAW2CMD_H_
#define _RAW2CMD_H_

#include "engine/game_env.h"
#include "engine/cmd.h"
class CmdReceiver;

// RawMsgStatus. 
custom_enum(RawMsgStatus, PROCESSED = 0, EXCEED_TICK, FAILED);

typedef function<CmdBPtr (const Unit&, char hotkey, const PointF& p, const UnitId &target_id, const GameEnv &)> EventResp; 

class RawToCmd {
private:
    // Internal status.
    PlayerId _player_id;
    set<UnitId> _sel_unit_ids;
    char _last_key;

    map<char, EventResp> _hotkey_maps;

    void select_new_units(const set<UnitId>& ids) {
        _sel_unit_ids = ids;
        _last_key = '~';
    }

    void clear_state() {
        _last_key = '~';
        _sel_unit_ids.clear();
    }

    void add_hotkey(const string &keys, EventResp func);
    void setup_hotkeys();

    static bool is_mouse_motion(char c) { return c == 'L' || c == 'R' || c == 'B'; } 

public:
    RawToCmd(PlayerId player_id = INVALID) : _player_id(player_id), _last_key('~') { setup_hotkeys(); }
    RawMsgStatus Process(const GameEnv &env, const string&s, CmdReceiver *receiver);

    void SetId(PlayerId id) { _player_id = id; clear_state();  }

    bool IsUnitSelected() const { return ! _sel_unit_ids.empty(); }
    bool IsSingleUnitSelected() const { return _sel_unit_ids.size() == 1; }
    bool IsUnitSelected(const UnitId& id) const { return _sel_unit_ids.find(id) != _sel_unit_ids.end(); }
    void ClearUnitSelection() { _sel_unit_ids.clear(); }
    const set<UnitId> &GetAllSelectedUnits() const { return _sel_unit_ids; }

    void NotifyDeleted(const UnitId& id) { 
        auto it = _sel_unit_ids.find(id);
        if (it != _sel_unit_ids.end()) {
            _sel_unit_ids.erase(it);
        }
    }
};

#endif

