#pragma once

#define TACTICAL_MOVE 100

class CmdTacticalMove : public CmdImmediate {
protected:
    PointF _p;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdTacticalMove() { }
    explicit CmdTacticalMove(UnitId id, const PointF& p) : CmdImmediate(id), _p(p) { }
    CmdType type() const override { return TACTICAL_MOVE; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdTacticalMove>(new CmdTacticalMove(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [p]: " << _p;
        return ss.str();
    }
    const PointF& p() const { return _p; }
    SERIALIZER_DERIVED(CmdTacticalMove, CmdImmediate, _p);
};

#define CREATE 101

class CmdCreate : public CmdImmediate {
protected:
    UnitType _build_type;
    PointF _p;
    PlayerId _player_id;
    int _resource_used;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdCreate() { }
    explicit CmdCreate(UnitId id, const UnitType& build_type, const PointF& p, const PlayerId& player_id, int resource_used = 0) : CmdImmediate(id), _build_type(build_type), _p(p), _player_id(player_id), _resource_used(resource_used) { }
    CmdType type() const override { return CREATE; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdCreate>(new CmdCreate(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [build_type]: " << _build_type << " [p]: " << _p << " [player_id]: " << _player_id << " [resource_used]: " << _resource_used;
        return ss.str();
    }
    const UnitType& build_type() const { return _build_type; }
    const PointF& p() const { return _p; }
    const PlayerId& player_id() const { return _player_id; }
    int resource_used() const { return _resource_used; }
    SERIALIZER_DERIVED(CmdCreate, CmdImmediate, _build_type, _p, _player_id, _resource_used);
};

#define REMOVE 102

class CmdRemove : public CmdImmediate {
protected:


    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdRemove() { }
    explicit CmdRemove(UnitId id) : CmdImmediate(id) { }
    CmdType type() const override { return REMOVE; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdRemove>(new CmdRemove(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() ;
        return ss.str();
    }

    SERIALIZER_DERIVED0(CmdRemove, CmdImmediate);
};

#define EMIT_BULLET 103

class CmdEmitBullet : public CmdImmediate {
protected:
    UnitId _target;
    PointF _p;
    int _att;
    float _speed;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdEmitBullet() { }
    explicit CmdEmitBullet(UnitId id, const UnitId& target, const PointF& p, int att, float speed) : CmdImmediate(id), _target(target), _p(p), _att(att), _speed(speed) { }
    CmdType type() const override { return EMIT_BULLET; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdEmitBullet>(new CmdEmitBullet(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [target]: " << _target << " [p]: " << _p << " [att]: " << _att << " [speed]: " << _speed;
        return ss.str();
    }
    const UnitId& target() const { return _target; }
    const PointF& p() const { return _p; }
    int att() const { return _att; }
    float speed() const { return _speed; }
    SERIALIZER_DERIVED(CmdEmitBullet, CmdImmediate, _target, _p, _att, _speed);
};

#define SAVE_MAP 104

class CmdSaveMap : public CmdImmediate {
protected:
    std::string _s;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdSaveMap() { }
    explicit CmdSaveMap(UnitId id, const std::string& s) : CmdImmediate(id), _s(s) { }
    CmdType type() const override { return SAVE_MAP; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdSaveMap>(new CmdSaveMap(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [s]: " << _s;
        return ss.str();
    }
    const std::string& s() const { return _s; }
    SERIALIZER_DERIVED(CmdSaveMap, CmdImmediate, _s);
};

#define LOAD_MAP 105

class CmdLoadMap : public CmdImmediate {
protected:
    std::string _s;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdLoadMap() { }
    explicit CmdLoadMap(UnitId id, const std::string& s) : CmdImmediate(id), _s(s) { }
    CmdType type() const override { return LOAD_MAP; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdLoadMap>(new CmdLoadMap(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [s]: " << _s;
        return ss.str();
    }
    const std::string& s() const { return _s; }
    SERIALIZER_DERIVED(CmdLoadMap, CmdImmediate, _s);
};

#define RANDOM_SEED 106

class CmdRandomSeed : public CmdImmediate {
protected:
    uint64_t _seed;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdRandomSeed() { }
    explicit CmdRandomSeed(UnitId id, const uint64_t& seed) : CmdImmediate(id), _seed(seed) { }
    CmdType type() const override { return RANDOM_SEED; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdRandomSeed>(new CmdRandomSeed(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [seed]: " << _seed;
        return ss.str();
    }
    const uint64_t& seed() const { return _seed; }
    SERIALIZER_DERIVED(CmdRandomSeed, CmdImmediate, _seed);
};

#define COMMENT 107

class CmdComment : public CmdImmediate {
protected:
    std::string _comment;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdComment() { }
    explicit CmdComment(UnitId id, const std::string& comment) : CmdImmediate(id), _comment(comment) { }
    CmdType type() const override { return COMMENT; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdComment>(new CmdComment(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [comment]: " << _comment;
        return ss.str();
    }
    const std::string& comment() const { return _comment; }
    SERIALIZER_DERIVED(CmdComment, CmdImmediate, _comment);
};

#define CDSTART 108

class CmdCDStart : public CmdImmediate {
protected:
    CDType _cd_type;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdCDStart() { }
    explicit CmdCDStart(UnitId id, const CDType& cd_type) : CmdImmediate(id), _cd_type(cd_type) { }
    CmdType type() const override { return CDSTART; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdCDStart>(new CmdCDStart(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [cd_type]: " << _cd_type;
        return ss.str();
    }
    const CDType& cd_type() const { return _cd_type; }
    SERIALIZER_DERIVED(CmdCDStart, CmdImmediate, _cd_type);
};

inline void reg_engine() {
    CmdTypeLookup::RegCmdType(TACTICAL_MOVE, "TACTICAL_MOVE");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdTacticalMove);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdTacticalMove);
    CmdTypeLookup::RegCmdType(CREATE, "CREATE");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdCreate);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdCreate);
    CmdTypeLookup::RegCmdType(REMOVE, "REMOVE");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdRemove);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdRemove);
    CmdTypeLookup::RegCmdType(EMIT_BULLET, "EMIT_BULLET");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdEmitBullet);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdEmitBullet);
    CmdTypeLookup::RegCmdType(SAVE_MAP, "SAVE_MAP");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdSaveMap);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdSaveMap);
    CmdTypeLookup::RegCmdType(LOAD_MAP, "LOAD_MAP");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdLoadMap);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdLoadMap);
    CmdTypeLookup::RegCmdType(RANDOM_SEED, "RANDOM_SEED");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdRandomSeed);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdRandomSeed);
    CmdTypeLookup::RegCmdType(COMMENT, "COMMENT");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdComment);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdComment);
    CmdTypeLookup::RegCmdType(CDSTART, "CDSTART");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdCDStart);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdCDStart);
}
