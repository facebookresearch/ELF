/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include <sstream>
#include "ai.h"
#include "elf/circular_queue.h"
#include "state_feature.h"




class TrainedAI : public AIWithComm {
public:
    using State = AIWithComm::State;
    using Action = AIWithComm::Action;
    using Data = typename AIWithComm::Data;

    TrainedAI() : _respect_fow(true), _recent_states(1) { }
    TrainedAI(const AIOptions &opt)
        : AIWithComm(opt.name), _respect_fow(opt.fow), _recent_states(opt.num_frames_in_state) {
      for (auto &v : _recent_states.v()) v.clear();
    }

    bool GameEnd() override;

protected:
     // Preload _preload;
    const bool _respect_fow;
    Preload_Train _preload;

    // History to send.
    CircularQueue<std::vector<float> > _recent_states;

    void compute_state(std::vector<float> *state);
    // Feature extraction.
    void extract(const State &, Data *data) override;
    bool handle_response(const State &, const Data &data, RTSMCAction *a) override;
    bool GatherInfo(const GameEnv &env,PlayerId _player_id);  // 收集执行命令需要的信息
};

