#pragma once

#define ATTACK 200

class CmdAttack : public CmdDurative {
protected:
    UnitId _target;
    int _round;

    bool run(const GameEnv& env, CmdReceiver *) override;

public:
    explicit CmdAttack() { }
    explicit CmdAttack(UnitId id, const UnitId& target, int round = 1) : CmdDurative(id), _target(target), _round(round) { }
    CmdType type() const override { return ATTACK; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdAttack>(new CmdAttack(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdDurative::PrintInfo() << " [target]: " << _target << " [round]: " << _round;
        return ss.str();
    }
    const UnitId& target() const { return _target; }
    int round() const { return _round; }
    SERIALIZER_DERIVED(CmdAttack, CmdDurative, _target, _round);
};

#define MOVE 201

class CmdMove : public CmdDurative {
protected:
    PointF _p;

    bool run(const GameEnv& env, CmdReceiver *) override;

public:
    explicit CmdMove() { }
    explicit CmdMove(UnitId id, const PointF& p) : CmdDurative(id), _p(p) { }
    CmdType type() const override { return MOVE; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdMove>(new CmdMove(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdDurative::PrintInfo() << " [p]: " << _p;
        return ss.str();
    }
    const PointF& p() const { return _p; }
    SERIALIZER_DERIVED(CmdMove, CmdDurative, _p);
};

#define BUILD 202

class CmdBuild : public CmdDurative {
protected:
    UnitType _build_type;
    PointF _p;
    int _state;

    bool run(const GameEnv& env, CmdReceiver *) override;

public:
    explicit CmdBuild() { }
    explicit CmdBuild(UnitId id, const UnitType& build_type, const PointF& p = PointF(), int state = 0) : CmdDurative(id), _build_type(build_type), _p(p), _state(state) { }
    CmdType type() const override { return BUILD; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdBuild>(new CmdBuild(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdDurative::PrintInfo() << " [build_type]: " << _build_type << " [p]: " << _p << " [state]: " << _state;
        return ss.str();
    }
    const UnitType& build_type() const { return _build_type; }
    const PointF& p() const { return _p; }
    int state() const { return _state; }
    SERIALIZER_DERIVED(CmdBuild, CmdDurative, _build_type, _p, _state);
};

#define GATHER 203

class CmdGather : public CmdDurative {
protected:
    UnitId _base;
    UnitId _resource;
    int _state;

    bool run(const GameEnv& env, CmdReceiver *) override;

public:
    explicit CmdGather() { }
    explicit CmdGather(UnitId id, const UnitId& base, const UnitId& resource, int state = 0) : CmdDurative(id), _base(base), _resource(resource), _state(state) { }
    CmdType type() const override { return GATHER; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdGather>(new CmdGather(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdDurative::PrintInfo() << " [base]: " << _base << " [resource]: " << _resource << " [state]: " << _state;
        return ss.str();
    }
    const UnitId& base() const { return _base; }
    const UnitId& resource() const { return _resource; }
    int state() const { return _state; }
    SERIALIZER_DERIVED(CmdGather, CmdDurative, _base, _resource, _state);
};

#define CIRCLE_MOVE 204

class CmdCircleMove : public CmdDurative {
protected:
    PointF _p;
    bool _isInCircle;
    PointF _towards;
    float _radians;
    bool _isReturn;

    bool run(const GameEnv& env, CmdReceiver *) override;

public:
    explicit CmdCircleMove() { }
    explicit CmdCircleMove(UnitId id, const PointF& p, const bool& isInCircle = false, const PointF& towards = PointF(), float radians = 0, const bool& isReturn = false) : CmdDurative(id), _p(p), _isInCircle(isInCircle), _towards(towards), _radians(radians), _isReturn(isReturn) { }
    CmdType type() const override { return CIRCLE_MOVE; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdCircleMove>(new CmdCircleMove(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdDurative::PrintInfo() << " [p]: " << _p << " [isInCircle]: " << _isInCircle << " [towards]: " << _towards << " [radians]: " << _radians << " [isReturn]: " << _isReturn;
        return ss.str();
    }
    const PointF& p() const { return _p; }
    const bool& isInCircle() const { return _isInCircle; }
    const PointF& towards() const { return _towards; }
    float radians() const { return _radians; }
    const bool& isReturn() const { return _isReturn; }
    SERIALIZER_DERIVED(CmdCircleMove, CmdDurative, _p, _isInCircle, _towards, _radians, _isReturn);
};

#define MELEE_ATTACK 205

class CmdMeleeAttack : public CmdImmediate {
protected:
    UnitId _target;
    int _att;
    float _att_r;
    int _round;
    PointF _p_tower;
    bool _isRandom;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdMeleeAttack() { }
    explicit CmdMeleeAttack(UnitId id, const UnitId& target, int att, float att_r = 0, int round = 1, const PointF& p_tower = PointF(), const bool& isRandom = false) : CmdImmediate(id), _target(target), _att(att), _att_r(att_r), _round(round), _p_tower(p_tower), _isRandom(isRandom) { }
    CmdType type() const override { return MELEE_ATTACK; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdMeleeAttack>(new CmdMeleeAttack(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [target]: " << _target << " [att]: " << _att << " [att_r]: " << _att_r << " [round]: " << _round << " [p_tower]: " << _p_tower << " [isRandom]: " << _isRandom;
        return ss.str();
    }
    const UnitId& target() const { return _target; }
    int att() const { return _att; }
    float att_r() const { return _att_r; }
    int round() const { return _round; }
    const PointF& p_tower() const { return _p_tower; }
    const bool& isRandom() const { return _isRandom; }
    SERIALIZER_DERIVED(CmdMeleeAttack, CmdImmediate, _target, _att, _att_r, _round, _p_tower, _isRandom);
};

#define ON_DEAD_UNIT 206

class CmdOnDeadUnit : public CmdImmediate {
protected:
    UnitId _target;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdOnDeadUnit() { }
    explicit CmdOnDeadUnit(UnitId id, const UnitId& target) : CmdImmediate(id), _target(target) { }
    CmdType type() const override { return ON_DEAD_UNIT; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdOnDeadUnit>(new CmdOnDeadUnit(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [target]: " << _target;
        return ss.str();
    }
    const UnitId& target() const { return _target; }
    SERIALIZER_DERIVED(CmdOnDeadUnit, CmdImmediate, _target);
};

#define HARVEST 207

class CmdHarvest : public CmdImmediate {
protected:
    UnitId _target;
    int _delta;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdHarvest() { }
    explicit CmdHarvest(UnitId id, const UnitId& target, int delta) : CmdImmediate(id), _target(target), _delta(delta) { }
    CmdType type() const override { return HARVEST; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdHarvest>(new CmdHarvest(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [target]: " << _target << " [delta]: " << _delta;
        return ss.str();
    }
    const UnitId& target() const { return _target; }
    int delta() const { return _delta; }
    SERIALIZER_DERIVED(CmdHarvest, CmdImmediate, _target, _delta);
};

#define CHANGE_PLAYER_RESOURCE 208

class CmdChangePlayerResource : public CmdImmediate {
protected:
    PlayerId _player_id;
    int _delta;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdChangePlayerResource() { }
    explicit CmdChangePlayerResource(UnitId id, const PlayerId& player_id, int delta) : CmdImmediate(id), _player_id(player_id), _delta(delta) { }
    CmdType type() const override { return CHANGE_PLAYER_RESOURCE; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdChangePlayerResource>(new CmdChangePlayerResource(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [player_id]: " << _player_id << " [delta]: " << _delta;
        return ss.str();
    }
    const PlayerId& player_id() const { return _player_id; }
    int delta() const { return _delta; }
    SERIALIZER_DERIVED(CmdChangePlayerResource, CmdImmediate, _player_id, _delta);
};

inline void reg_engine_specific() {
    CmdTypeLookup::RegCmdType(ATTACK, "ATTACK");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdAttack);
    SERIALIZER_ANCHOR_FUNC(CmdDurative, CmdAttack);
    CmdTypeLookup::RegCmdType(MOVE, "MOVE");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdMove);
    SERIALIZER_ANCHOR_FUNC(CmdDurative, CmdMove);
    CmdTypeLookup::RegCmdType(BUILD, "BUILD");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdBuild);
    SERIALIZER_ANCHOR_FUNC(CmdDurative, CmdBuild);
    CmdTypeLookup::RegCmdType(GATHER, "GATHER");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdGather);
    SERIALIZER_ANCHOR_FUNC(CmdDurative, CmdGather);
    CmdTypeLookup::RegCmdType(CIRCLE_MOVE, "CIRCLE_MOVE");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdCircleMove);
    SERIALIZER_ANCHOR_FUNC(CmdDurative, CmdCircleMove);
    CmdTypeLookup::RegCmdType(MELEE_ATTACK, "MELEE_ATTACK");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdMeleeAttack);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdMeleeAttack);
    CmdTypeLookup::RegCmdType(ON_DEAD_UNIT, "ON_DEAD_UNIT");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdOnDeadUnit);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdOnDeadUnit);
    CmdTypeLookup::RegCmdType(HARVEST, "HARVEST");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdHarvest);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdHarvest);
    CmdTypeLookup::RegCmdType(CHANGE_PLAYER_RESOURCE, "CHANGE_PLAYER_RESOURCE");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdChangePlayerResource);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdChangePlayerResource);
}
