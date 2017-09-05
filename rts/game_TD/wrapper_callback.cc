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

typedef TDTrainedAI TrainAIType;
static AI *get_ai(const AIOptions &opt, Context::AIComm *ai_comm) {
    // std::cout << "AI type = " << ai_type << " Backup AI type = " << backup_ai_type << std::endl;
    if (opt.type == "AI_TD_BUILT_IN") return new TDBuiltInAI(opt, nullptr);
    else if (opt.type == "AI_TD_NN") return new TrainAIType(opt, nullptr, ai_comm);
    else return nullptr;
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
    }

    for (AI *ai : ais) game->AddBot(ai);

}

void WrapperCallbacks::OnEpisodeStart(int k, std::mt19937 *rng, RTSGame*) {
    (void)k;
    (void)rng;
}
