#pragma once
#include "selene.h"
#include "cmd_receiver.h"
#include "unit.h"
#include "player.h"
#include "lua.hpp"
#include "game_env.h"

// Lua Wrapper for Unit.
class LuaUnit {
public:
    explicit LuaUnit(UnitId id, const Player& player, const Unit *u = nullptr) 
        : id_(id), player_(&player), unit_(u) { 
    }
    explicit LuaUnit() : id_(INVALID), player_(nullptr), unit_(nullptr) {
    }

    LuaUnit &operator=(const LuaUnit &u) {
        player_ = u.player_;
        unit_ = u.unit_;
        id_ = u.id_;
        return *this;
    }

    const Unit& get() const { return *unit_; }

    bool isdead() const { return unit_ == nullptr; }
    PointF p() const { return unit_->GetPointF(); }
    const PointF &p_ref() const { return unit_->GetPointF(); }

    const UnitProperty &pty() const { return unit_->GetProperty(); }

    int hp() const { return pty()._hp; }
    int att() const { return pty()._att; }
    int def() const { return pty()._def; }
    int att_r() const { return  pty()._att_r; }

    PlayerId player_id() const { return unit_->GetPlayerId(); }

    bool CDExpired(Tick tick, int cd_type) {
        return unit_->GetProperty().CD((CDType)cd_type).Passed(tick); 
    }

    bool CanSee(const LuaUnit &u) const { 
        return player_->GetPrivilege() != PV_NORMAL || player_->FilterWithFOW(*u.unit_);
    }

    string info() const { 
        stringstream ss;
        if (isdead()) {
            ss << "[" << id_ << "] Unit is dead";
        } else {
            ss << "[" << id_ << "] " << p() << " hp: " << hp() << ", att: " << att() << ", def: " << def();
        }
        return ss.str();
    }

private:
    UnitId id_;
    const Player *player_;
    const Unit *unit_;
};

template <typename... Ts>
class CmdDurativeLuaT;

// LuaWrapper of the environment.
class LuaEnv {
public:
    LuaEnv(CmdReceiver *receiver);

    LuaEnv &SetGameEnv(const GameEnv &env) {
        env_ = &env;
        return *this;
    }

    template <typename... Ts>
    void Run(const CmdDurativeLuaT<Ts...> &cmd) { 
        if (cmd.just_started()) {
            // Initialize its LUA component.
            _init_cmd(cmd);
        }
        cmd_ = &cmd;
        unit_ = GetUnit(cmd_->id());
        s_["g_run_cmd"](cmd.cmd_id(), *this);
    } 

    void CDStart(int cd_type);
    double DistSqr(const PointF &target_p);
    LuaUnit GetSelf() const { return unit_; }

    PointF FindNearbyEmptyPlace(const PointF &p);
    void MoveTowardsTarget(const PointF &target_p);
    void MoveTowards(const LuaUnit &u);

    void SendCmdMeleeAttack(UnitId target, int att);
    void SendCmdEmitBullet(UnitId target, int att);
    void SendCmdCreate(int build_type, const PointF &build_p);

    void HarvestResource(UnitId res, int delta);
    void ChangeResource(int delta);
    int UnitCost(int unit_type);

    LuaUnit GetUnit(UnitId id) const;

private:
    const GameEnv *env_ = nullptr;
    CmdReceiver *cmd_receiver_ = nullptr;
    const CmdDurative *cmd_ = nullptr;

    LuaUnit unit_;

    sel::State s_;

    template <typename... Ts>
    void _init_cmd(const CmdDurativeLuaT<Ts...> &cmd) {
        s_["g_init_cmd_start"](cmd.cmd_id(), cmd.name());
        _init_cmd_impl<0, Ts...>(cmd);
    }

    template <std::size_t I, typename... Ts>
    std::enable_if<I < sizeof... (Ts), void>  _init_cmd_impl(const CmdDurativeLuaT<Ts...> &cmd) {
        s_["g_init_cmd"](cmd.cmd_id(), cmd.key(I), std::get<I>(cmd.data()));
        _init_cmd_impl<I + 1, Ts...>(cmd);
    }

    template <std::size_t I, typename... Ts>
    std::enable_if<I == sizeof... (Ts), void>  _init_cmd_impl(const CmdDurativeLuaT<Ts...> &) {
    }
};

#include "cmd_receiver.h"

template <typename... Ts>
class CmdDurativeLuaT : public CmdDurative {
public:
    using CmdDurativeLua = CmdDurativeLuaT<Ts...>;

    explicit CmdDurativeLuaT(const std::string& name, const std::vector<std::string> &keys, Ts && ...args) 
        : _name(name), _keys(keys), _data(args...) {
    }

    bool run(const GameEnv& env, CmdReceiver *receiver) override {
        LuaEnv &lua_env = receiver->GetLuaEnv().SetGameEnv(env);

        // Run lua script.
        return lua_env.Run(*this);
    }
    const std::string &name() const { return _name; }
    const std::string &key(std::size_t i) const { return _keys[i]; }
    const std::tuple<Ts...> &data() const { return _data; }
    
    SERIALIZER_DERIVED(CmdDurativeLua, CmdBase);
    SERIALIZER_ANCHOR(CmdDurativeLua);
    UNIQUE_PTR_COMPARE(CmdDurativeLua);

protected:
    std::string _name;
    std::vector<std::string> _keys;
    std::tuple<Ts...> _data;
};

template <typename... Ts>
SERIALIZER_ANCHOR_INIT(CmdDurativeLuaT<Ts...>);

