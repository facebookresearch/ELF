#pragma once

#include <sstream>

#include "elf/utils.h"
#include "engine/game_state.h"
#include "engine/custom_enum.h"
#include "python_options.h"

using namespace std;

#define _OFFSET(_c, _x, _y, _m) (((_c) * _m.GetYSize() + (_y)) * _m.GetXSize() + (_x))
#define _XY(loc, m) ((loc) % m.GetXSize()), ((loc) / m.GetXSize())

#define NUM_RES_SLOT 5

struct MCExtractorOptions {
    bool use_time_decay = true;
    bool save_prev_seen_units = false;
    bool attach_complete_info = false;

    string info() const {
        stringstream ss;
        ss << "Use time decay: " << elf_utils::print_bool(use_time_decay) << endl;
        ss << "Save prev seen units: " << elf_utils::print_bool(save_prev_seen_units) << endl;
        ss << "Attach complete info: " << elf_utils::print_bool(attach_complete_info) << endl;
        return ss.str();
    }

    REGISTER_PYBIND_FIELDS(use_time_decay, save_prev_seen_units, attach_complete_info);
};

custom_enum(MCExtractorUsageType, ORIGINAL = 0, BUILD_HISTORY, PREV_SEEN, ONLY_PREV_SEEN);

struct MCExtractorUsageOptions {
    MCExtractorUsageType type = ORIGINAL;

    string info() const {
        std::stringstream ss;
        ss << type;
        return ss.str();
    }

    void Set(const string &s) {
        type = _string2MCExtractorUsageType(s);
    }

    REGISTER_PYBIND;
};

class Extractor {
public:
    Extractor(int base_addr) : base_addr_(base_addr) { }
    int Get(int key) const { return get_offset(key) + base_addr_; }
    virtual int size() const = 0;

private:
    int base_addr_;

protected:
    virtual int get_offset(int key) const = 0;
};

class ExtractorSpan : public Extractor {
public:
    ExtractorSpan(int base_addr, int num_val, int span = 1)
        : Extractor(base_addr), num_val_(num_val), span_(span) {
    }

    int size() const override { return num_val_; }

private:
    int num_val_;
    int span_;

protected:
    int get_offset(int key) const override { return min(key / span_, num_val_ - 1); }
};

class ExtractorSeq : public Extractor {
public:
    ExtractorSeq(int base_addr, std::initializer_list<int> seqs)
        : Extractor(base_addr), seqs_(seqs) {
    }

    int size() const override { return seqs_.size() + 1; }

private:
    vector<int> seqs_;

protected:
    int get_offset(int key) const override {
        for (size_t i = 0; i < seqs_.size(); ++i) {
            if (key < seqs_[i]) return i;
        }
        return seqs_.size();
    }
};

class MCExtractorInfo {
public:
    enum FeatureType {
        AFFILIATION = 0,
        HP_RATIO,
        BASE_HP_RATIO,
        HISTORY_DECAY,
        NUM_FEATURE
    };

    MCExtractorInfo() {
        Reset(MCExtractorOptions());
    }

    void Reset(const MCExtractorOptions &opt) {
        extractors_.clear();

        int num_unit_type = GameDef::GetNumUnitType();
        total_dim_ = 0;
        total_dim_ += _add_extractor("UnitType", new ExtractorSpan(total_dim_, num_unit_type));
        total_dim_ += _add_extractor("Feature", new ExtractorSpan(total_dim_, NUM_FEATURE));

        std::initializer_list<int> ticks = { 200, 500, 1000, 2000, 5000, 10000 };

        if (opt.use_time_decay) {
            total_dim_ += _add_extractor("HistBin", new ExtractorSeq(total_dim_, ticks));
        }

        if (opt.save_prev_seen_units) {
            total_dim_ += _add_extractor("UnitTypePrevSeen", new ExtractorSpan(total_dim_, num_unit_type));
            total_dim_ += _add_extractor("HistBinPrevSeen", new ExtractorSeq(total_dim_, ticks));
        }

        total_dim_ += _add_extractor("Resource", new ExtractorSpan(total_dim_, NUM_RES_SLOT, 50));
    }

    int size() const { return total_dim_; }
    const Extractor *get(const std::string &name) const {
        auto it = extractors_.find(name);
        if (it == extractors_.end()) return nullptr;
        else return it->second.get();
    }

private:
    int total_dim_;
    map<string, unique_ptr<Extractor>> extractors_;

    int _add_extractor(const std::string &name, Extractor *e) {
        extractors_[name].reset(e);
        return e->size();
    }
};

class MCExtractor {
public:
    static void Init(const MCExtractorOptions &opt) {
        info_.Reset(opt);
        attach_complete_info_ = opt.attach_complete_info;
    }

    static void InitUsage(const MCExtractorUsageOptions &opt) {
        usage_ = opt;
    }

    // static const MCExtractorInfo &GetInfo() { return info_; }
    //
    static size_t Size() {
        if (attach_complete_info_) return info_.size() * 2;
        else return info_.size();
    }

    static void Extract(const RTSState &s, PlayerId player_id, bool respect_fow, std::vector<float> *state);
    static void SaveInfo(const RTSState &s, PlayerId player_id, GameState *gs);

private:
    static MCExtractorInfo info_;
    static MCExtractorUsageOptions usage_;
    static bool attach_complete_info_;

    static void extract(const RTSState &s, PlayerId player_id, bool respect_fow, float *state);
};
