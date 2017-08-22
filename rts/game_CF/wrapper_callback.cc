/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "wrapper_callback.h"
#include "engine/cmd.h"
#include "engine/cmd.gen.h"
#include "engine/cmd_specific.gen.h"
#include "cmd_specific.gen.h"
#include "ai.h"

typedef FlagTrainedAI TrainAIType;
static AI *get_ai(const AIOptions &opt, Context::AIComm *ai_comm) {
    // std::cout << "AI type = " << ai_type << " Backup AI type = " << backup_ai_type << std::endl;
    switch (opt.type) {
       case AI_FLAG_SIMPLE:
           return new FlagSimpleAI(opt, nullptr);
       case AI_FLAG_NN:
           {
           AI *backup_ai = nullptr;
           AIOptions backup_ai_options;
           backup_ai_options.fs = opt.fs;
           if (opt.backup == AI_FLAG_SIMPLE) backup_ai = new FlagSimpleAI(opt, nullptr);
           return new TrainAIType(opt, nullptr, ai_comm, backup_ai);
           }
       default:
           return nullptr;
    }
}

void WrapperCallbacks::GlobalInit() {
    reg_engine();
    reg_engine_specific();
    reg_cf_specific();
}

void WrapperCallbacks::initialize_ai_comm(Context::AIComm &ai_comm) {
    auto &hstate = ai_comm.info().data;
    hstate.InitHist(_context_options.T);
    for (auto &item : hstate.v()) {
        item.Init(_game_idx, GameDef::GetNumAction());
    }
}

void WrapperCallbacks::OnGameOptions(RTSGameOptions *rts_options) {
    rts_options->handicap_level = _options.handicap_level;
}

void WrapperCallbacks::OnGameInit(RTSGame *game) {
    // std::cout << "Initialize opponent" << std::endl;
    std::vector<AI *> ais;
    for (const AIOptions &ai_opt : _options.ai_options) {
        Context::AIComm *ai_comm = new Context::AIComm(_game_idx, _comm);
        _ai_comms.emplace_back(ai_comm);
        initialize_ai_comm(*ai_comm);
        ais.push_back(get_ai(ai_opt, ai_comm));
    }

    // std::cout << "Initialize ai" << std::endl;
    // Used to pick the AI to change the parameters.
    _ai = ais[0];

    // Shuffle the bot.
    if (_options.shuffle_player) {
        std::mt19937 g(_game_idx);
        std::shuffle(ais.begin(), ais.end(), g);
    } else if (_options.reverse_player) {
        std::reverse(ais.begin(), ais.end());
    }

    for (AI *ai : ais) game->AddBot(ai);
}

void WrapperCallbacks::OnEpisodeStart(int k, std::mt19937 *rng, RTSGame*) {
    if (k > 0) {
        // Decay latest_start.
        _latest_start *= _options.latest_start_decay;
    }

    TrainAIType *ai_dyn = dynamic_cast<TrainAIType *>(_ai);
    if (ai_dyn == nullptr) return;

    // Random tick, max 1000
    Tick default_ai_end_tick = (*rng)() % (int(_latest_start + 0.5) + 1);
    ai_dyn->SetBackupAIEndTick(default_ai_end_tick);

}
