/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "save2json.h"
#include "engine/cmd.h"
#include "engine/cmd.gen.h"
#include "engine/cmd_specific.gen.h"
#include "engine/game_env.h"
#include "engine/ai.h"
#include "engine/cmd.h"

static inline void set_p(const PointF &p, json *location) {
    (*location)["x"] = p.x;
    (*location)["y"] = p.y;
}

static inline void set_cmd(const CmdDurative *_c, json *cmd) {
    const CmdDurative &c = *_c;
    (*cmd)["cmd"] = CmdTypeLookup::idx2str(c.type());
    (*cmd)["id"] = c.id();
    (*cmd)["state"] = 0;

    if (c.type() == ATTACK) {
        const CmdAttack &tmp = dynamic_cast<const CmdAttack &>(c);
        (*cmd)["target_id"] = tmp.target();
    } else if (c.type() == MOVE) {
        const CmdMove &tmp = dynamic_cast<const CmdMove &>(c);
        set_p(tmp.p(), &(*cmd)["p"]);
    } else if (c.type() == GATHER) {
        const CmdGather &tmp = dynamic_cast<const CmdGather &>(c);
        (*cmd)["target_id"] = tmp.resource();
        (*cmd)["state"] = tmp.state();
    } else if (c.type() == BUILD) {
        const CmdBuild &tmp = dynamic_cast<const CmdBuild &>(c);
        (*cmd)["state"] = tmp.state();
    }
}

void save2json::SetTick(Tick tick, json *game) {
    (*game)["tick"] = tick;
}

void save2json::SetWinner(PlayerId id, json *game) {
    (*game)["winner"] = id;
}

void save2json::SetPlayerId(PlayerId id, json *game) {
    (*game)["player_id"] = id;
}

void save2json::SetSpectator(bool is_spectator, json *game) {
    (*game)["spectator"] = is_spectator;
}

void save2json::SetTermination(bool t, json *game) {
    (*game)["terminated"] = t;
}

void save2json::SetGameCounter(int game_counter, json *game) {
    (*game)["game_counter"] = game_counter;
}

void save2json::Save(const RTSMap& m, json *game) {
    json rts_map;
    rts_map["width"] = m.GetXSize();
    rts_map["height"] = m.GetYSize();
    json slots;
    for (int y = 0; y < m.GetYSize(); y ++) {
        for (int x = 0; x < m.GetXSize(); x ++) {
           Loc loc = m.GetLoc(Coord(x, y));
           slots.push_back(m(loc).type);
        }
    }
    rts_map["slots"] = slots;
    (*game)["rts_map"] = rts_map;
}

void save2json::SavePlayerMap(const Player& player, json *game) {
    json rts_map;
    const RTSMap &m = player.GetMap();
    rts_map["width"] = m.GetXSize();
    rts_map["height"] = m.GetYSize();
    json slots;
    for (int y = 0; y < m.GetYSize(); y ++) {
        for (int x = 0; x < m.GetXSize(); x ++) {
            Loc loc = m.GetLoc(Coord(x, y, 0));
            const Fog &f = player.GetFog(loc);

            Terrain t = FOG;
            if (f.CanSeeTerrain()) {
                t = m(loc).type;
            } else {
                // Add prev seen units.
                for (const auto &u : f.seen_units()) {
                    Save(u, nullptr, &rts_map);
                }
            }

            slots.push_back(t);
        }
    }
    rts_map["slots"] = slots;
    (*game)["rts_map"] = rts_map;
}

void save2json::SaveGameDef(const GameDef& gamedef, json *game) {
    json json_gd;
    json u_def;
    for (auto& ut : gamedef._units) {
      json unit;
      for (const auto& allowed_cmd : ut._allowed_cmds) {
        json cmd;
        cmd["id"] = allowed_cmd;
        unit["allowed_cmds"].push_back(cmd);
      }
      for (const auto& mult : ut._attack_multiplier) {
        unit["attack_multiplier"].push_back(mult);
      }
      for (const auto& build_skill : ut.GetBuildSkills()) {
        json skill;
        skill["unit_type"] = build_skill.GetUnitType();
        skill["hotkey"] = build_skill.GetHotKey();
        skill["price"] = gamedef.unit(build_skill.GetUnitType()).GetUnitCost();
        unit["build_skills"].push_back(skill);
      }
      u_def.push_back(unit);
    }
    json_gd["units"] = u_def;
    (*game)["gamedef"] = json_gd;
}

void save2json::SaveStats(const Player& player, json *game) {
    // Save the information for player.
    json pp;
    pp["player_id"] = player.GetId();
    pp["resource"] = player.GetResource();
    (*game)["players"].push_back(pp);
}

/*
void save2json::Save(const AI &bot, json *game) {
    json mbot;
    vector<int> selected = bot.GetAllSelectedUnits();
    for (int id : selected) {
        mbot["selected_units"].push_back(id);
    }
    vector<int> state = bot.GetState();
    for (int id : state) {
        mbot["state"].push_back(id);
    }
    (*game)["bots"].push_back(mbot);
}
*/

void save2json::Save(const Unit& unit, const CmdReceiver *receiver, json *game) {
    json u;
    u["id"] = unit.GetId();
    u["player_id"] = unit.GetPlayerId();

    const UnitProperty& prop = unit.GetProperty();
    u["att"] = prop._att;
    u["att_r"] = prop._att_r;
    u["vis_r"] = prop._vis_r;
    u["def"] = prop._def;
    u["hp"] = prop._hp;
    u["max_hp"] = prop._max_hp;
    u["speed"] = prop._speed;
    u["unit_type"] = unit.GetUnitType();

    set_p(unit.GetPointF(), &u["p"]);
    set_p(unit.GetLastPointF(), &u["last_p"]);

    // Save commands.
    if (receiver != nullptr) {
        const CmdDurative *cmd = receiver->GetUnitDurativeCmd(unit.GetId());
        if (cmd != nullptr) {
            set_cmd(cmd, &u["cmd"]);
        } else {
            u["cmd"]["cmd"] = "I";
            u["cmd"]["id"] = unit.GetId();
            u["cmd"]["state"] = 0;
        }
    }

    // Set cds.
    for (int i = 0; i < NUM_COOLDOWN; ++i) {
        json cd;
        CDType t = (CDType)i;
        const Cooldown &cd_ref = prop.CD(t);

        cd["cd"] = cd_ref._cd;
        cd["last"] = cd_ref._last;
        cd["name"] = ::_CDType2string(t);
        u["cds"].push_back(cd);
    }
    (*game)["units"].push_back(u);
}

void save2json::Save(const Bullet& bullet, json *game) {
    json bb;
    bb["id_from"] = bullet.GetIdFrom();
    bb["state"] = ::_BulletState2string(bullet.GetState());
    set_p(bullet.GetPointF(), &bb["p"]);
    (*game)["bullets"].push_back(bb);
}

void save2json::SaveCmd(const CmdReceiver &receiver, PlayerId player_id, json *game) {
    const vector<CmdDurative*> &added_cmds = receiver.GetHistoryAtCurrentTick();
    for (const auto cmd : added_cmds) {
        if (player_id == INVALID || Player::ExtractPlayerId(cmd->id()) == player_id) {
            set_cmd(cmd, &(*game)["new_cmd"]);
        }
    }
}
