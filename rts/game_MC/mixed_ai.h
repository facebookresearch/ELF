#pragma once

#include "ai.h"

class MixedAI : public AI {
public:
    using State = AI::State;
    using Action = AI::Action;

    MixedAI(const AIOptions &opt) : AI(opt.name) {
          if (opt.args != "") {
              // TODO: Get some library to do this.
              std::string backup_ai_name;
              for (const auto &kv : _parse(opt.args)) {
                  if (kv.first == "start") _latest_start = std::stoi(kv.second);
                  else if (kv.first == "decay") _latest_start_decay = std::stof(kv.second);
                  else if (kv.first == "backup") {
                      AIOptions opt2;
                      opt2.fs = opt.fs;
                      if (kv.second == "AI_SIMPLE" || kv.second == "ai_simple") {
                          // std::cout << "Initialize backup as ai_simple" << std::endl;
                          _backup_ai.reset(new SimpleAI(opt2));
                      }
                      else if (kv.second == "AI_TOWER_DEFENSE" || kv.second == "ai_tower_defense") {
                          // std::cout << "Initialize backup as ai_hit_and_run" << std::endl;
                          _backup_ai.reset(new TowerDefenseAI(opt2));
                      }
                  } else {
                      std::cout << "Unrecognized (key, value) = (" << kv.first << "," << kv.second << ")" << std::endl;
                  }
              }
              // std::cout << "Latest start = " << _latest_start << " decay = " << _latest_start_decay << std::endl;
              if (_backup_ai.get() != nullptr) {
                  _backup_ai->SetId(id());
              }
          }
          _rng.seed(time(NULL));
    }

    void SetMainAI(AI *main_ai) {
        _main_ai.reset(main_ai);
        _main_ai->SetId(id());
    }

    bool Act(const State &s, RTSMCAction *a, const std::atomic_bool *done) override {
        if (_backup_ai != nullptr && s.GetTick() < _backup_ai_tick_thres) {
            return _backup_ai->Act(s, a, done);
        } else {
            return _main_ai->Act(s, a, done);
        }
    }

    bool GameEnd() override {
        AI::GameEnd();

        // Always ended with main_ai.
        bool res = _main_ai->GameEnd();

        // Decay latest_start.
        _latest_start *= _latest_start_decay;

        // Random tick, max 1000
        // TODO this might not be random enough?
        _backup_ai_tick_thres = _rng() % (int(_latest_start + 0.5) + 1);
        return res;
    }

protected:
    // Backup AI.
    // Used when we want the default ai to play for a while and then TrainedAI can take over.
    std::unique_ptr<AI> _backup_ai;
    std::unique_ptr<AI> _main_ai;

    Tick _backup_ai_tick_thres = 0;
    std::mt19937 _rng;

    // Latest start of backup AI. When training, before each game starts,
    // we will sample a tick ~ Uniform(0, latest_start) and run backup AI
    // until that tick, then switch to NN-AI.
    int _latest_start = 0;

    // Decay of latest_start after each game.
    // After each game, latest_start is decayed by latest_start_decay.
    float _latest_start_decay = 0;

    static std::map<std::string, std::string> _parse(const std::string& args) {
        std::map<std::string, std::string> kvmap;
        for (const auto &item : CmdLineUtils::split(args, '|')) {
            std::vector<std::string> kv = CmdLineUtils::split(item, '/');
            if (kv.size() != 2) continue;
            kvmap.insert(std::make_pair(kv[0], kv[1]));
        }
        return kvmap;
    }

    void on_set_id() override {
        this->AI::on_set_id();
        if (_backup_ai != nullptr) _backup_ai->SetId(id());
        if (_main_ai != nullptr) _main_ai->SetId(id());
    }
};
