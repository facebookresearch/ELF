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
#include "rule_ai.h"
#include "mcts.h"
#include "mixed_ai.h"

static AI *get_ai(const AIOptions &opt, Context::AIComm *ai_comm) {
    // std::cout << "AI type = " << ai_type << " Backup AI type = " << backup_ai_type << std::endl;
    if (opt.type == "AI_SIMPLE") return new SimpleAI(opt);
    else if (opt.type == "AI_HIT_AND_RUN") return new HitAndRunAI(opt);
    else if (opt.type == "AI_NN") {
        TrainedAI *main_ai = new TrainedAI(opt);
        main_ai->InitAIComm(ai_comm);

        MixedAI *ai = new MixedAI(opt);
        ai->SetMainAI(main_ai);
        return ai;
    // } else if (opt.type == "AI_MCTS") {
    //    mcts::TSOptions options;
    //    return MCTSRTSAI(options);
    } else if (opt.type == "AI_REDUCE_MCTS") {
        mcts::TSOptions options;
        return new MCTSRTSReducedAI(options);
    } else return nullptr;
    /*
       std::string prompt = "Unknown ai_type! ai_type: " + std::to_string(ai_type) + " backup_ai_type: " + std::to_string(backup_ai_type);
       std::cout << prompt << std::endl;
       throw std::range_error(prompt);
       */
}

void WrapperCallbacks::initialize_ai_comm(Context::AIComm &ai_comm) {
    auto &hstate = ai_comm.info().data;
    hstate.InitHist(_context_options.T);
    for (auto &item : hstate.v()) {
        item.Init(_game_idx, GameDef::GetNumAction(), _options.max_unit_cmd, _options.map_size_x, _options.map_size_y, CmdInput::CI_NUM_CMDS, GameDef::GetNumUnitType());
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
    // Shuffle the bot.
    if (_options.shuffle_player) {
        std::mt19937 g(_game_idx);
        std::shuffle(ais.begin(), ais.end(), g);
    }

    for (AI *ai : ais) game->AddBot(ai);
}

void WrapperCallbacks::OnEpisodeStart(int k, std::mt19937 *rng, RTSGame *game) {
    (void)k;
    (void)rng;
    (void)game;
}
