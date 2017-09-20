/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include "engine/ai.h"
#include "engine/cmd_util.h"
#include "engine/cmd_interface.h"
#include "elf/circular_queue.h"
#include "python_options.h"
#include "mc_rule_actor.h"
#define NUM_RES_SLOT 5

using Comm = Context::Comm;
using AIComm = AICommT<Comm>;
using Data = typename AIComm::Data;

class AIBase : public AIWithComm<AIComm> {
protected:
    bool _respect_fow;

    // History to send.
    mutable CircularQueue<std::vector<float>> _recent_states;

    // Feature extraction.
    void save_structured_state(const GameEnv &env, Data *data) const override;

    void compute_state(const GameEnv &env, std::vector<float> *state) const;

public:
    AIBase() : _recent_states(1) { }
    AIBase(const AIOptions &opt, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : AIWithComm<AIComm>(opt.name, opt.fs, receiver, ai_comm), _respect_fow(opt.fow), _recent_states(opt.num_frames_in_state) {
      for (auto &v : _recent_states.v()) v.clear();
    }

    void Reset() override;
};

// Simple AI, rule-based AI for Mini-RTS
class SimpleAI : public AIBase {
private:
    MCRuleActor _mc_rule_actor;
    bool on_act(const GameEnv &env) override;
    RuleActor *rule_actor() override { return &_mc_rule_actor; }

public:
    SimpleAI() {
    }
    SimpleAI(const AIOptions &opt, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : AIBase(opt, receiver, ai_comm)  {
    }

    SERIALIZER_DERIVED(SimpleAI, AIBase, _state);
};

// HitAndRun AI, rule-based AI for Mini-RTS
class HitAndRunAI : public AIBase {
private:
    MCRuleActor _mc_rule_actor;
    bool on_act(const GameEnv &env) override;
    RuleActor *rule_actor() override { return &_mc_rule_actor; }

public:
    HitAndRunAI() {
    }
    HitAndRunAI(const AIOptions &opt, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : AIBase(opt, receiver, ai_comm) {
    }

    SERIALIZER_DERIVED(HitAndRunAI, AIBase, _state);
};

// TrainedAI2 for MiniRTS,  connected with a python wrapper / ELF.
class TrainedAI2 : public AIBase {
private:
    // Backup AI.
    // Used when we want the default ai to play for a while and then TrainedAI can take over.
    Tick _backup_ai_tick_thres;
    std::unique_ptr<AI> _backup_ai;
    MCRuleActor _mc_rule_actor;

    std::vector<CmdInput> _cmd_inputs;

    // Latest start of backup AI. When training, before each game starts,
    // we will sample a tick ~ Uniform(0, latest_start) and run backup AI
    // until that tick, then switch to NN-AI.
    int _latest_start;

    // Decay of latest_start after each game.
    // After each game, latest_start is decayed by latest_start_decay.
    float _latest_start_decay;

    enum ActionType { ACTION_GLOBAL = 0, ACTION_REGIONAL, ACTION_UNIT_CMD };
    ActionType _action_type = ACTION_GLOBAL;

protected:
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

    RuleActor *rule_actor() override { return &_mc_rule_actor; }

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
    TrainedAI2(const AIOptions &opt, CmdReceiver *receiver, AIComm *ai_comm)
      : AIBase(opt, receiver, ai_comm), _backup_ai_tick_thres(0), _latest_start(0), _latest_start_decay(0) {
          if (ai_comm == nullptr) {
              throw std::range_error("TrainedAI2: ai_comm cannot be nullptr!");
          }
          if (opt.args != "") {
              // TODO: Get some library to do this.
              std::string backup_ai_name;
              for (const auto &kv : _parse(opt.args)) {
                  if (kv.first == "start") _latest_start = std::stoi(kv.second);
                  else if (kv.first == "decay") _latest_start_decay = std::stof(kv.second);
                  else if (kv.first == "action_type") {
                      if (kv.second == "global9") _action_type = ACTION_GLOBAL;
                      else if (kv.second == "regional9") _action_type = ACTION_REGIONAL;
                      else if (kv.second == "unit_cmd") _action_type = ACTION_UNIT_CMD;
                      else {
                          std::cout << "Unknown action type: " << kv.second << std::endl;
                      }
                  }
                  else if (kv.first == "backup") {
                      AIOptions opt2;
                      opt2.fs = opt.fs;
                      if (kv.second == "AI_SIMPLE" || kv.second == "ai_simple") {
                          // std::cout << "Initialize backup as ai_simple" << std::endl;
                          _backup_ai.reset(new SimpleAI(opt2, nullptr));
                      }
                      else if (kv.second == "AI_HIT_AND_RUN" || kv.second == "ai_hit_and_run") {
                          // std::cout << "Initialize backup as ai_hit_and_run" << std::endl;
                          _backup_ai.reset(new HitAndRunAI(opt2, nullptr));
                      }
                  } else {
                      std::cout << "Unrecognized (key, value) = (" << kv.first << "," << kv.second << ")" << std::endl;
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
        AIBase::Reset();
        if (_ai_comm->seq_info().game_counter > 0) {
            // Decay latest_start.
            _latest_start *= _latest_start_decay;
        }

        // Random tick, max 1000
        _backup_ai_tick_thres = _ai_comm->gen()() % (int(_latest_start + 0.5) + 1);
    }
};

