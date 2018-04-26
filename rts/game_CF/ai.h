/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "engine/ai.h"
#include "engine/cmd_util.h"
#include "python_options.h"
#include "cf_rule_actor.h"

using Comm = Context::Comm;
using AIComm = AICommT<Comm>;
using Data = typename AIComm::Data;

class AIBase : public AIWithComm<AIComm> {
protected:
    bool _respect_fow;

    int last_r0 = 0;
    int last_r1 = 0;
    int last_x = 9;

    void on_save_data(Data *data) override {
        GameState *game = &data->newest();
        if (game->winner != INVALID) return;

        // assign partial rewards
        if (game->r0 > last_r0) {
            game->last_r = 0.2;
            last_r0 = game->r0;
        } else if (game->r1 > last_r1) {
            game->last_r = -0.2;
            last_r1 = game->r1;
        } else {
            game->last_r = float(last_x - game->flag_x) / 100;
            last_x = game->flag_x;
        }
    }

    // Feature extraction.
    void save_structured_state(const GameEnv &env, Data *data) const override;

public:
    AIBase() { }
    AIBase(const AIOptions &opt, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : AIWithComm<AIComm>(opt.name, opt.fs, receiver, ai_comm), _respect_fow(opt.fow) {
    }
};

// FlagSimple AI, rule-based AI for Capture the Flag
class FlagSimpleAI : public AIBase {
private:
    bool on_act(const GameEnv &env) override;
    CFRuleActor _cf_rule_actor;

    RuleActor *rule_actor() override { return &_cf_rule_actor; }
public:
    FlagSimpleAI() {
    }
    FlagSimpleAI(const AIOptions &opt, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
      : AIBase(opt, receiver, ai_comm) {
    }

    SERIALIZER_DERIVED(FlagSimpleAI, AI, _state);
};

// FlagTrainedAI for Capture the Flag,  connected with a python wrapper / ELF.
class FlagTrainedAI : public AIBase {
private:
    // Backup AI.
    // Used when we want the default ai to play for a while and then TrainedAI can take over.
    Tick _backup_ai_tick_thres;
    std::unique_ptr<AI> _backup_ai;
    CFRuleActor _cf_rule_actor;
    int _latest_start;
    float _latest_start_decay;

    bool on_act(const GameEnv &env) override;

    void on_set_id(PlayerId id) override {
        this->AIBase::on_set_id(id);
        if (_backup_ai != nullptr) _backup_ai->SetId(id);
    }

    void on_set_cmd_receiver(CmdReceiver *receiver) override {
        this->AIBase::on_set_cmd_receiver(receiver);
        if (_backup_ai != nullptr) _backup_ai->SetCmdReceiver(receiver);
    }

    void on_save_data(Data *data) override {
        this->AIBase::on_save_data(data);
        data->newest().ai_start_tick = _backup_ai_tick_thres;
    }

    bool need_structured_state(Tick tick) const override {
        if (_backup_ai != nullptr && tick < _backup_ai_tick_thres) {
            // We just use the backup AI.
            return false;
        } else {
            return true;
        }
    }

    RuleActor *rule_actor() override { return &_cf_rule_actor; }
    static std::map<std::string, std::string> _parse(const std::string& args) {
        std::map<std::string, std::string> kvmap;
        for (const auto &item : CmdLineUtils::split(args, '|')) {
            std::vector<std::string> kv = CmdLineUtils::split(item, '/');
            if (kv.size() != 2) continue;
            kvmap.insert(std::make_pair(kv[0], kv[1]));
        }
        return kvmap;
    }

public:
    FlagTrainedAI(const AIOptions &opt, CmdReceiver *receiver, AIComm *ai_comm)
      : AIBase(opt, receiver, ai_comm), _backup_ai_tick_thres(0) {
          if (ai_comm == nullptr) {
              throw std::range_error("FlagTrainedAI: ai_comm cannot be nullptr!");
          }
          if (opt.args != "") {
              // TODO: Get some library to do this.
              std::string backup_ai_name;
              for (const auto &kv : _parse(opt.args)) {
                  if (kv.first == "start") _latest_start = std::stoi(kv.second);
                  else if (kv.first == "decay") _latest_start_decay = std::stof(kv.second);
                  else if (kv.first == "backup") {
                      AIOptions opt2;
                      opt2.fs = opt.fs;
                      _backup_ai.reset(new FlagSimpleAI(opt2, nullptr));
                  }
              }
              // std::cout << "Latest start = " << _latest_start << " decay = " << _latest_start_decay << std::endl;
              if (_backup_ai.get() != nullptr) {
                  _backup_ai->SetId(GetId());
                  _backup_ai->SetCmdReceiver(_receiver);
              }
          }
    }

    void Reset() override {
        AIWithComm::Reset();
        if (_ai_comm->seq_info().game_counter > 0) {
            // Decay latest_start.
            _latest_start *= _latest_start_decay;
        }

        // Random tick, max 1000
        _backup_ai_tick_thres = _ai_comm->gen()() % (int(_latest_start + 0.5) + 1);
    }
};
