#pragma once

#define GENERATE_MAP 1000

class CmdGenerateMap : public CmdImmediate {
protected:
    int _num_obstacles;
    int _init_resource;

    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdGenerateMap() { }
    explicit CmdGenerateMap(UnitId id, int num_obstacles, int init_resource) : CmdImmediate(id), _num_obstacles(num_obstacles), _init_resource(init_resource) { }
    CmdType type() const override { return GENERATE_MAP; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdGenerateMap>(new CmdGenerateMap(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() << " [num_obstacles]: " << _num_obstacles << " [init_resource]: " << _init_resource;
        return ss.str();
    }
    int num_obstacles() const { return _num_obstacles; }
    int init_resource() const { return _init_resource; }
    SERIALIZER_DERIVED(CmdGenerateMap, CmdImmediate, _num_obstacles, _init_resource);
};

#define GENERATE_UNIT 1001

class CmdGenerateUnit : public CmdImmediate {
protected:


    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdGenerateUnit() { }
    explicit CmdGenerateUnit(UnitId id) : CmdImmediate(id) { }
    CmdType type() const override { return GENERATE_UNIT; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdGenerateUnit>(new CmdGenerateUnit(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() ;
        return ss.str();
    }

    SERIALIZER_DERIVED0(CmdGenerateUnit, CmdImmediate);
};

#define GAME_START_SPECIFIC 1002

class CmdGameStartSpecific : public CmdImmediate {
protected:


    bool run(GameEnv* env, CmdReceiver *) override;

public:
    explicit CmdGameStartSpecific() { }
    explicit CmdGameStartSpecific(UnitId id) : CmdImmediate(id) { }
    CmdType type() const override { return GAME_START_SPECIFIC; }
    std::unique_ptr<CmdBase> clone() const override {
        auto res = std::unique_ptr<CmdGameStartSpecific>(new CmdGameStartSpecific(*this));
        // copy_to(*res);
        return std::move(res);
    }
    string PrintInfo() const override {
        std::stringstream ss;
        ss << this->CmdImmediate::PrintInfo() ;
        return ss.str();
    }

    SERIALIZER_DERIVED0(CmdGameStartSpecific, CmdImmediate);
};

inline void reg_minirts_specific() {
    CmdTypeLookup::RegCmdType(GENERATE_MAP, "GENERATE_MAP");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdGenerateMap);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdGenerateMap);
    CmdTypeLookup::RegCmdType(GENERATE_UNIT, "GENERATE_UNIT");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdGenerateUnit);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdGenerateUnit);
    CmdTypeLookup::RegCmdType(GAME_START_SPECIFIC, "GAME_START_SPECIFIC");
    SERIALIZER_ANCHOR_FUNC(CmdBase, CmdGameStartSpecific);
    SERIALIZER_ANCHOR_FUNC(CmdImmediate, CmdGameStartSpecific);
}
