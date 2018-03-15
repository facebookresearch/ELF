#pragma once
#include "selene.h"
#include "unit.h"
#include "player.h"
#include "lua.hpp"

// Lua Wrapper for Unit.
class LuaUnit {
public:
    explicit LuaUnit(Tick tick, UnitId id, const Player& player, const Unit *u = nullptr) 
        : tick_(tick), id_(id), player_(&player), unit_(u) { 
    }
    explicit LuaUnit() : tick_(0), id_(INVALID), player_(nullptr), unit_(nullptr) {
    }

    LuaUnit &operator=(const LuaUnit &u) {
        tick_ = u.tick_;
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
bool CDExpired(int cd_type) {
        // std::cout << unit_->GetProperty().Draw(tick_) << std::endl;
        return unit_->GetProperty().CD((CDType)cd_type).Passed(tick_); 
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
    Tick tick_;
    UnitId id_;
    const Player *player_;
    const Unit *unit_;
};

template <typename... Ts>
class CmdDurativeLuaT;

class CmdReceiver;
class GameEnv;

enum CmdReturnLua { LUA_CMD_NORMAL = 0, LUA_CMD_FAILED = 1, LUA_CMD_COMPLETE = 2 };

// LuaWrapper of the environment.
class LuaEnv {
public:
    LuaEnv();

    LuaEnv &Set(const GameEnv &env, CmdReceiver *receiver) {
        env_ = &env;
        receiver_ = receiver;
        return *this;
    }

    template <typename... Ts>
    CmdReturnLua Run(const CmdDurativeLuaT<Ts...> &cmd) { 
        if (cmd.just_started()) {
            // Initialize its LUA component.
            _init_cmd(cmd);
        }
        cmd_ = &cmd;
        unit_ = GetUnit(cmd_->id());
        int ret = s_["g_run_cmd"](cmd.cmd_id(), *this);
        // std::cout << "Return from LUA (in LuaEnv) ret = " << ret << std::endl;
        return static_cast<CmdReturnLua>(ret);
    } 

    void CDStart(int cd_type);
    double DistSqr(const PointF &target_p);
    LuaUnit GetSelf() const { return unit_; }
    int Tick() const;

    PointF FindNearbyEmptyPlace(const LuaUnit& u, const PointF &p);
    double MoveTowardsTarget(const PointF &target_p);
    double MoveTowards(const LuaUnit &u);

    void SendCmdMeleeAttack(UnitId target, int att);
    void SendCmdEmitBullet(UnitId target, int att);
    void SendCmdCreate(int build_type, const PointF &build_p);

    void HarvestResource(UnitId res, int delta);
    void ChangeResource(int delta);
    int UnitCost(int unit_type);

    LuaUnit GetUnit(UnitId id) const;

private:
    const GameEnv *env_ = nullptr;
    CmdReceiver *receiver_ = nullptr;
    const CmdDurative *cmd_ = nullptr;

    LuaUnit unit_;

    sel::State s_;

    void _register_obj(int cmd_id, const std::string &k, const int &v) {
        s_["g_cmds"][cmd_id][k] = v;
    }

    void _register_obj(int cmd_id, const std::string &k, const double &v) {
        s_["g_cmds"][cmd_id][k] = v;
    }

    void _register_obj(int cmd_id, const std::string &k, const PointF &v) {
        s_["g_cmds"][cmd_id][k].SetObj(v, 
            "self", &PointF::self,
            "isvalid", &PointF::IsValid, 
            "info", &PointF::info);
    }

    template <typename... Ts>
    void _init_cmd(const CmdDurativeLuaT<Ts...> &cmd) {
        s_["g_init_cmd_start"](cmd.cmd_id(), cmd.name());
        _init_cmd_impl<0, Ts...>(cmd);
    }

    template <std::size_t I, typename... Ts>
    typename std::enable_if<I < sizeof... (Ts), void>::type  _init_cmd_impl(const CmdDurativeLuaT<Ts...> &cmd) {
        _register_obj(cmd.cmd_id(), cmd.key(I), std::get<I>(cmd.data()));
        _init_cmd_impl<I + 1, Ts...>(cmd);
    }

    template <std::size_t I, typename... Ts>
    typename std::enable_if<I == sizeof... (Ts), void>::type  _init_cmd_impl(const CmdDurativeLuaT<Ts...> &) {
    }
};

LuaEnv &_get_lua_env(const GameEnv&, CmdReceiver *);

template <typename... Ts>
class CmdDurativeLuaT : public CmdDurative {
public:
    using CmdDurativeLua = CmdDurativeLuaT<Ts...>;

    explicit CmdDurativeLuaT(const std::string& name, const std::vector<std::string> &keys, const Ts & ...args) 
        : _name(name), _keys(keys), _data(args...) {
    }
    explicit CmdDurativeLuaT() : _name("undefined") { 
    }
    explicit CmdDurativeLuaT(const CmdDurativeLua &c) 
        : CmdDurative(c), _name(c._name), _keys(c._keys), _data(c._data) {
    }

    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdDurative::PrintInfo() << "[lua] ";
        for (const auto &k : _keys) {
            ss << k << " ";
        }
        return ss.str();
    }

    std::unique_ptr<CmdBase> clone() const override { 
        return std::unique_ptr<CmdDurativeLua>(new CmdDurativeLua(*this)); 
    }
    CmdType type() const override { return CMD_DURATIVE_LUA; }

    bool run(const GameEnv& env, CmdReceiver *receiver) override {
        LuaEnv &lua_env = _get_lua_env(env, receiver);

        // Run lua script.
        CmdReturnLua ret = lua_env.Run(*this);
        if (ret == LUA_CMD_COMPLETE) {
            SetDone();
            return true;
        } else return ret == LUA_CMD_NORMAL;
    }
    const std::string &name() const { return _name; }
    const std::string &key(std::size_t i) const { return _keys[i]; }
    const std::tuple<Ts...> &data() const { return _data; }
    std::tuple<Ts...> &data() { return _data; }
    
    SERIALIZER_DERIVED(CmdDurativeLua, CmdBase, _name, _keys, _data);
    SERIALIZER_ANCHOR(CmdDurativeLua);
    UNIQUE_PTR_COMPARE(CmdDurativeLua);

protected:
    std::string _name;
    std::vector<std::string> _keys;
    std::tuple<Ts...> _data;

};

void reg_engine_cmd_lua();
