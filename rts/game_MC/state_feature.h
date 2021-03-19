/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

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

class Preload_Train {
public:
    enum Result { NOT_READY = -1, OK = 0, NO_BASE };

private:
    vector<vector<const Unit*> > _my_troops;  //我方单位
    vector<pair<float, const Unit*> > _enemy_troops_in_range; // 视野内的敌方单位
    vector<const Unit*> _all_my_troops;

    UnitId _base_id;
    PointF _base_loc;
    int _base_hp;

    const Unit *_base = nullptr;
    PlayerId _player_id = INVALID;
    int _num_unit_type = 0;
    Result _result = NOT_READY;
    

    static bool InCmd(const CmdReceiver &receiver, const Unit &u, CmdType cmd) {
        const CmdDurative *c = receiver.GetUnitDurativeCmd(u.GetId());
        return (c != nullptr && c->type() == cmd);
    }

    void collect_stats(const GameEnv &env, int player_id);

public:
    Preload_Train() { }

    void GatherInfo(const GameEnv &env, int player_id);
    Result GetResult() const { return _result; }
    
    
    //CmdBPtr GetAccackEnemyUnitCmd(UnitId t_id) const { return _A(t_id);}
    
    
    // 随机获取攻击范围内的指定类型目标
    // custom_enum(FlightType, INVALID_FLIGHTTYPE = -1, FLIGHT_NORMAL = 0, FLIGHT_BASE, FLIGHT_TOWER, FLIGHT_FAKE, NUM_FLIGHT);  // 飞机种类
    
    const PointF &BaseLoc() const { return _base_loc; }
    const Unit *Base() const { return _base; }
    const int BaseHP() const {return _base_hp;}
    
    float DistanceToBase(const PointF &u_loc) const;
    PointF VectorToBase(const PointF& u_loc) const;
    

    const vector<vector<const Unit*> > &MyTroops() const { return _my_troops; }
    const vector<pair<float, const Unit*> > &EnemyTroopsInRange() const { return _enemy_troops_in_range; }
    const vector<const Unit*> &AllMyTroops() const { return _all_my_troops; }
    string EnemyInfo() const {
        stringstream ss;
        for(int i=0;i<_enemy_troops_in_range.size();++i){
            ss<<_enemy_troops_in_range[i].second->GetId()<<"   ";
        }
        return ss.str();
    }
};

// MCExtractor设置
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
    ExtractorSeq(int base_addr, std::initializer_list<int> seqs) // leak?
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
        Reset(MCExtractorOptions());  // leak?
    }

    void Reset(const MCExtractorOptions &opt) {
        //std::cout<<"============Reset==============="<<std::endl;
        extractors_.clear();

        int num_unit_type = GameDef::GetNumUnitType();  
        total_dim_ = 0;
        total_dim_ += _add_extractor("UnitType", new ExtractorSpan(total_dim_, num_unit_type));  // 6
        total_dim_ += _add_extractor("Feature", new ExtractorSpan(total_dim_, NUM_FEATURE));     // 4

        std::initializer_list<int> ticks = { 200, 500, 1000, 2000, 5000, 10000 };

        if (opt.use_time_decay) {
            // std::cout<<"===opt.use_time_decay==="<<std::endl;
            // std::cout<<"total dim = "<<total_dim_<<std::endl;
            //total_dim_ += _add_extractor("HistBin", new ExtractorSeq(total_dim_, ticks));   // 6+1 leak?
            // std::cout<<"after add total dim = "<<total_dim_<<std::endl;
        }

        if (opt.save_prev_seen_units) {
            total_dim_ += _add_extractor("UnitTypePrevSeen", new ExtractorSpan(total_dim_, num_unit_type));
            total_dim_ += _add_extractor("HistBinPrevSeen", new ExtractorSeq(total_dim_, ticks));
        }

       // total_dim_ += _add_extractor("Resource", new ExtractorSpan(total_dim_, NUM_RES_SLOT, 50));  // 5
    }

    int size() const { return total_dim_; }
    const Extractor *get(const std::string &name) const {
        auto it = extractors_.find(name);
        if (it == extractors_.end()) return nullptr;
        else return it->second.get();
    }

private:
    int total_dim_;
    map<string, unique_ptr<Extractor>> extractors_;   //

    int _add_extractor(const std::string &name, Extractor *e) {
        extractors_[name].reset(e);
        return e->size();
    }
};

// class MCExtractor {
// public:
//     static void Init(const MCExtractorOptions &opt) {
//         info_.Reset(opt);
//         attach_complete_info_ = opt.attach_complete_info;
//     }

//     static void InitUsage(const MCExtractorUsageOptions &opt) {
//         usage_ = opt;
//     }

//     // static const MCExtractorInfo &GetInfo() { return info_; }
//     //
//     static size_t Size() {
//         if (attach_complete_info_) return info_.size() * 2;
//         else return info_.size();
//     }

//     static void Extract(const RTSState &s, PlayerId player_id, bool respect_fow, std::vector<float> *state);
//     static void SaveInfo(const RTSState &s, PlayerId player_id, GameState *gs);

// private:
   
//     static MCExtractorInfo info_;
//     static MCExtractorUsageOptions usage_;
//     static bool attach_complete_info_;

//     static void extract(const RTSState &s, PlayerId player_id, bool respect_fow, float *state);
// };



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

    static void Extract(const RTSState &s, const Preload_Train &preload, PlayerId player_id, bool respect_fow, GameState *gs);

    

private:
    
    static MCExtractorInfo info_;
    static MCExtractorUsageOptions usage_;
    static bool attach_complete_info_;

    static void extract(const RTSState &s, PlayerId player_id, bool respect_fow, float *state);

    static void extract_global(const Preload_Train &preload,std::vector<float> &s_global);
    static void extract_base  (const Preload_Train &preload,std::vector<float> &s_base);
    static void extract_radar(const GameEnv &env, const Preload_Train &preload,std::vector<float> &s_radar);
    static void extract_tower(Tick tick,const Preload_Train &preload,std::vector<float> &s_tower);
    static void extract_enemy(const GameEnv &env, PlayerId player_id, const Preload_Train &preload,std::vector<float> &s_enemy);
};










