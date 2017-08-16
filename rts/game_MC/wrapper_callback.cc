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
static AI *get_ai(int game_idx, int frame_skip, int ai_type, int backup_ai_type, const PythonOptions &options, Context::AIComm *ai_comm) {
    // std::cout << "AI type = " << ai_type << " Backup AI type = " << backup_ai_type << std::endl;
    switch (ai_type) {
       case AI_SIMPLE:
           return new SimpleAI(INVALID, frame_skip, nullptr);
       case AI_HIT_AND_RUN:
           return new HitAndRunAI(INVALID, frame_skip, nullptr);
       case AI_NN:
           return new TrainAIType(INVALID, frame_skip, options.with_fow, nullptr, ai_comm, get_ai(game_idx, frame_skip, backup_ai_type, AI_INVALID, options, ai_comm));
       default:
           return nullptr;
           /*
           std::string prompt = "Unknown ai_type! ai_type: " + std::to_string(ai_type) + " backup_ai_type: " + std::to_string(backup_ai_type);
           std::cout << prompt << std::endl;
           throw std::range_error(prompt);
           */
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
    // std::cout << "Initialize opponent" << std::endl;
    Context::AIComm * ai_comm = _ai_comms[0];
    Context::AIComm *ai_comm_oppo = _ai_comms.size() >= 2 ? _ai_comms[1] : nullptr;
    _opponent = get_ai(_game_idx, _options.frame_skip_opponent, _options.opponent_ai_type, _options.backup_opponent_ai_type, _options, ai_comm_oppo);

    // std::cout << "Initialize ai" << std::endl;
    _ai = get_ai(_game_idx, _options.frame_skip_ai, _options.ai_type, _options.backup_ai_type, _options, ai_comm);

    // AI at position 0
    // std::cout << "Add AI at position 0" << std::endl;
    game->AddBot(_ai);
    // Opponent at position 1
    // std::cout << "Add AI at position 1" << std::endl;
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

        Context::AIComm *ai_comm_oppo = _ai_comms.size() >= 2 ? _ai_comms[1] : nullptr;

        if (use_simple) {
            _opponent = get_ai(INVALID, _options.frame_skip_opponent, AI_SIMPLE, AI_INVALID, _options, ai_comm_oppo);
        } else {
            _opponent = get_ai(INVALID, _options.frame_skip_opponent, AI_HIT_AND_RUN, AI_INVALID, _options, ai_comm_oppo);
        }
        game->AddBot(_opponent);
    }
}
