/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "game_env.h"
#include "cmd.h"
#include "cmd_specific.gen.h"
#include "cmd_util.h"

#include <vector>
#include <string>

// Cmd interface. From vector<string> to executable command.
// For example:
// Unit = id | (float, float)
// Loc = (float, float)
// move Unit Loc
// attack Unit1 Unit2(target)
// gather Unit1 ResourceUnit
// build Unit1 UnitTypeToBuild Loc

// To prevent parsing, we also provide structured interface
struct CmdInput {
    enum CmdInputType {CI_INVALID = -1, CI_MOVE = 0, CI_ATTACK, CI_GATHER, CI_BUILD, CI_NUM_CMDS};

    CmdInputType type;
    UnitId id, target, base;
    PointF p;
    UnitType build_type;

    // ready signal.
    // When not ready, id/target/base are not usable and should be used after ApplyEnv().
    bool ready = false;
    PointF unit_loc;

    // By default this is ready.
    CmdInput(CmdInputType t, UnitId id, const PointF &p, UnitId target = INVALID, UnitId base = INVALID, UnitType build_type = INVALID_UNITTYPE)
        : type(t), id(id), target(target), base(base), p(p), build_type(build_type), ready(true) {
    }
    CmdInput() : type(CI_INVALID), id(INVALID), target(INVALID), base(INVALID), ready(false) { }

    // Not ready and need to call ApplyEnv().
    CmdInput(float unit_loc_x, float unit_loc_y, float target_loc_x, float target_loc_y, int cmd_type, int build_tp)
        : type((CmdInputType)cmd_type), id(INVALID), target(INVALID), base(INVALID), p(target_loc_x, target_loc_y), build_type((UnitType)build_tp), ready(false), unit_loc(unit_loc_x, unit_loc_y) {
    }

    std::string info() const {
        std::stringstream ss;
        ss << "[" << (ready ? "True" : "False") << "] type: " << type << " id: " << id << " target: " << target << " base: " << base << " p: " << p << " build_type: " << build_type;
        return ss.str();
    }

    void ApplyEnv(const GameEnv &env) {
        // Check unit id.
        id = env.GetMap().GetClosestUnitId(unit_loc, 1.0);  
        target = env.GetMap().GetClosestUnitId(p, 1.0);
        base = INVALID;
        if (type == CI_GATHER && id != INVALID) base = env.FindClosestBase(Player::ExtractPlayerId(id));
        ready = true;
    }

    CmdBPtr GetCmd() const {
        if (id == INVALID) return CmdBPtr();
        switch (type) {
            case CI_MOVE:
                if (! p.IsInvalid())
                    return CmdBPtr(new CmdMove(id, p));
                break;
            case CI_ATTACK:
                if (target != INVALID && Player::ExtractPlayerId(id) != Player::ExtractPlayerId(target))
                    // Forbid friendly fire.
                    return CmdBPtr(new CmdAttack(id, target));
                break;
            case CI_GATHER:
                if (target != INVALID && base != INVALID)
                    return CmdBPtr(new CmdGather(id, base, target));
                break;
            case CI_BUILD:
                if (build_type != RESOURCE)
                    return CmdBPtr(new CmdBuild(id, build_type, p));
                break;
            default:
                return CmdBPtr();
        }
        return CmdBPtr();
    }
};

class CmdInterface {
public:
    using Tokens = std::vector<std::string>;

    CmdInterface(const GameEnv &env) : _env(env), _success(true) { }

    CmdInput Parse(const Tokens& tokens) {
        if (tokens.empty()) return CmdInput();
        _success = true;
        if (tokens[0] == "move") return on_move(tokens);
        else if (tokens[0] == "attack") return on_attack(tokens);
        else if (tokens[0] == "gather") return on_gather(tokens);
        else if (tokens[0] == "build") return on_build(tokens);
        else return CmdInput();
    }

    bool GetLastSuccess() const { return _success; }

private:
    const GameEnv &_env;
    bool _success;

    PointF parse_loc(const Tokens& tokens, int *idx) {
        // loc is in the form of (2.35,4.5). Note that there is no space allowed
        const auto& token = tokens[*idx];
        (*idx) ++;

        if (token[0] != '(' || token[token.size() - 1] != ')') { _success = false; return PointF(); }
        Tokens items = CmdLineUtils::split(token.substr(1, token.size() - 2), ',');
        if (items.size() != 2) { _success = false; return PointF(); }
        return PointF(std::stof(items[0]), std::stof(items[1]));
    }

    UnitId parse_unit(const Tokens& tokens, int *idx) {
        UnitId id;
        if (tokens[*idx][0] == '(') {
            PointF loc = parse_loc(tokens, idx);
            // From loc to unit.
            id = _env.GetMap().GetClosestUnitId(loc, 1.0);
        } else {
            // Unit id.
            const auto &token = tokens[*idx];
            (*idx) ++;
            id = std::stoi(token);
        }
        return id;
    }

    UnitType parse_unittype(const Tokens& tokens, int *idx) {
        const auto &token = tokens[*idx];
        (*idx) ++;
        return _string2UnitType(token);
    }

    CmdInput on_move(const Tokens& tokens) {
        int idx = 1;
        UnitId id = parse_unit(tokens, &idx);
        PointF loc = parse_loc(tokens, &idx);
        return CmdInput(CmdInput::CI_MOVE, id, loc);
    }

    CmdInput on_attack(const Tokens& tokens) {
        int idx = 1;
        UnitId id = parse_unit(tokens, &idx);
        UnitId target = parse_unit(tokens, &idx);
        return CmdInput(CmdInput::CI_ATTACK, id, PointF(), target);
    }

    CmdInput on_gather(const Tokens& tokens) {
        int idx = 1;
        UnitId id = parse_unit(tokens, &idx);
        UnitId target = parse_unit(tokens, &idx);
        UnitId base = _env.FindClosestBase(Player::ExtractPlayerId(id));
        return CmdInput(CmdInput::CI_GATHER, id, PointF(), target, base);
    }

    CmdInput on_build(const Tokens& tokens) {
        int idx = 1;
        UnitId id = parse_unit(tokens, &idx);
        UnitType unittype = parse_unittype(tokens, &idx);
        PointF loc = parse_loc(tokens, &idx);
        return CmdInput(CmdInput::CI_BUILD, id, loc, INVALID, INVALID, unittype);
    }
};
