/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "wrapper_callback.h"
#include "../engine/cmd.h"
#include "../engine/cmd.gen.h"
#include "../engine/cmd_specific.gen.h"
#include "cmd_specific.gen.h"
#include "ai.h"

typedef TrainedAI2 TrainAIType;
static AI *get_ai(int game_idx, int frame_skip, int ai_type, int backup_ai_type,
    const PythonOptions &options, Context::AIComm *input_ai_comm, bool use_ai_comm = false /*, int *opponent_ai_type = INVALID*/) {
    AIComm *ai_comm = use_ai_comm ? input_ai_comm : nullptr;

    switch (ai_type) {
       case AI_SIMPLE:
           return new SimpleAI(INVALID, frame_skip, nullptr, ai_comm);
       case AI_HIT_AND_RUN:
           return new HitAndRunAI(INVALID, frame_skip, nullptr, ai_comm);
       case AI_NN:
           return new TrainAIType(INVALID, frame_skip, options.with_fow, nullptr, ai_comm, get_ai(game_idx, frame_skip, backup_ai_type, AI_INVALID, options, input_ai_comm));
       /*case AI_MCTS_VALUE:
       {
           AI *ai = new MCTS_VALUE_AI(INVALID, frame_skip, nullptr, ai_comm, options.mcts_threads, options.mcts_rollout_per_thread);
           switch (opponent_ai_type) {
               case AI_SIMPLE:
                   ai->SetFactory([&](int r) -> AI* { return new SimpleAI(INVALID, r, nullptr, nullptr);});
                   break;
               case AI_HIT_AND_RUN:
                   ai->SetFactory([&](int r) -> AI* { return new HitAndRunAI(INVALID, r, nullptr, nullptr);});
                   break;
           }
           return ai;
       }*/
       default:
           throw std::range_error("Unknown ai_type! ai_type: " + std::to_string(ai_type) +
                   " backup_ai_type: " + std::to_string(backup_ai_type) + " use_ai_comm: " + std::to_string(use_ai_comm));
    }
}

void WrapperCallbacks::GlobalInit() {
    reg_engine();
    reg_engine_specific();
    reg_minirts_specific();
}

void WrapperCallbacks::OnGameOptions(RTSGameOptions *rts_options) {
    rts_options->handicap_level = _options.handicap_level;
}

void WrapperCallbacks::OnGameInit(RTSGame *game) {
    _opponent = get_ai(INVALID, _options.frame_skip_opponent, _options.opponent_ai_type, AI_INVALID, _options, _ai_comm);
    _ai = get_ai(_game_idx, _options.frame_skip_ai, _options.ai_type, _options.backup_ai_type, _options, _ai_comm, true/*, _options.opponent_ai_type*/);

    // AI at position 0
    game->AddBot(_ai);
    // Opponent at position 1
    game->AddBot(_opponent);

    _latest_start = _options.latest_start;
    _simple_ratio = _options.simple_ratio;
}

void WrapperCallbacks::OnEpisodeStart(int k, std::mt19937 *rng, RTSGame *game) {
    if (k > 0) {
        if ((_options.ratio_change != 0) && (_simple_ratio != 50)) {
            _simple_ratio += _options.ratio_change;
        }
        // Decay latest_start.
        _latest_start *= _options.latest_start_decay;
    }

    // [TODO]: Not a good design.
    if (_options.ai_type != AI_NN) return;

    // Random tick, max 1000
    Tick default_ai_end_tick = (*rng)() % (int(_latest_start + 0.5) + 1);
    TrainAIType *ai_dyn = dynamic_cast<TrainAIType *>(_ai);
    if (ai_dyn == nullptr) throw std::range_error("The type of AI is wrong!");
    ai_dyn->SetBackupAIEndTick(default_ai_end_tick);
    if (_options.simple_ratio != -1) {
        bool use_simple = int((*rng)() % 100) < _simple_ratio;
        game->RemoveBot();

        if (use_simple) {
            _opponent = get_ai(INVALID, _options.frame_skip_opponent, AI_SIMPLE, AI_INVALID, _options, _ai_comm);
        } else {
            _opponent = get_ai(INVALID, _options.frame_skip_opponent, AI_HIT_AND_RUN, AI_INVALID, _options, _ai_comm);
        }
        game->AddBot(_opponent);
    }
}
