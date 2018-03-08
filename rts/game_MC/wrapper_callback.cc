/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "elf/utils.h"
#include "wrapper_callback.h"
#include "engine/cmd.h"
#include "engine/cmd.gen.h"
#include "engine/cmd_specific.gen.h"
#include "cmd_specific.gen.h"
#include "rule_ai.h"
#include "mixed_ai.h"
#include "trainable_ai.h"
#include "mcts.h"

static AI *get_ai(int game_idx, const mcts::TSOptions &mcts_opt, const AIOptions &opt, Context::AIComm *ai_comm) {
    // std::cout << "AI type = " << ai_type << " Backup AI type = " << backup_ai_type << std::endl;
    if (opt.type == "AI_SIMPLE") return new SimpleAI(opt);
    else if (opt.type == "AI_TOWER_DEFENSE") return new TowerDefenseAI(opt);
    else if (opt.type == "AI_NN") {
        TrainedAI *main_ai = new TrainedAI(opt);
        main_ai->InitAIComm(ai_comm);

        MixedAI *ai = new MixedAI(opt);
        ai->SetMainAI(main_ai);
        return ai;
    } else if (opt.type == "AI_MCTS") {
        mcts::TSOptions opt = mcts_opt;
        opt.save_tree_filename = mcts_opt.save_tree_filename + "_" + std::to_string(game_idx) + ".txt";
        MCTSRTSAI *ai = new MCTSRTSAI(opt);
        return ai;
    } else if (opt.type == "AI_REDUCED_MCTS") {
        // cout << opt.info() << endl;
        mcts::TSOptions opt = mcts_opt;
        opt.save_tree_filename = mcts_opt.save_tree_filename + "_" + std::to_string(game_idx) + ".txt";
        MCTSRTSReducedAI *ai = new MCTSRTSReducedAI(ai_comm, opt);
        return ai;
    } else {
        cout << "Unknown opt.type: " + opt.type << endl;
        return nullptr;
    }
    /*
       std::string prompt = "Unknown ai_type! ai_type: " + std::to_string(ai_type) + " backup_ai_type: " + std::to_string(backup_ai_type);
       std::cout << prompt << std::endl;
       throw std::range_error(prompt);
       */
}

void WrapperCallbacks::initialize_ai_comm(Context::AIComm &ai_comm, const std::map<std::string, int> *more_params) {
    int reduced_dim = elf_utils::map_get(*more_params, "reduced_dim", 1);
    auto &hstate = ai_comm.info().data;
    hstate.InitHist(_context_options.T);
    for (auto &item : hstate.v()) {
        // [TODO] This design is really not good..
        item.Init(_game_idx, GameDef::GetNumAction(), _options.max_unit_cmd, _options.map_size_x, _options.map_size_y,
                  CmdInput::CI_NUM_CMDS, GameDef::GetNumUnitType(), reduced_dim);
    }
}

void WrapperCallbacks::OnGameOptions(RTSGameOptions *rts_options) {
    rts_options->handicap_level = _options.handicap_level;
}

void WrapperCallbacks::OnGameInit(RTSGame *game, const std::map<std::string, int> *more_params) {
    // std::cout << "Initialize opponent" << std::endl;
    std::vector<AI *> ais;
    for (const AIOptions &ai_opt : _options.ai_options) {
        Context::AIComm *ai_comm = new Context::AIComm(_game_idx, _comm);
        _ai_comms.emplace_back(ai_comm);
        initialize_ai_comm(*ai_comm, more_params);
        ais.push_back(get_ai(_game_idx, _context_options.mcts_options, ai_opt, ai_comm));
    }

    // std::cout << "Initialize ai" << std::endl;
    // Shuffle the bot.
    std::vector<int> orders(ais.size());
    for (size_t i = 0; i < ais.size(); ++i) orders[i] = i;

    if (_options.shuffle_player) {
        std::mt19937 g(_game_idx);
        std::shuffle(orders.begin(), orders.end(), g);
        // cout << "[" << _game_idx << "] Shuffle is done: " << orders[0] << " " << orders[1] << endl;
    }

    for (size_t i = 0; i < ais.size(); ++i) {
        int idx = orders[i];
        game->AddBot(ais[idx], _options.ai_options[idx].fs);
        game->GetState().AppendPlayer("player " + std::to_string(idx));
    }
}

void WrapperCallbacks::OnEpisodeStart(int k, std::mt19937 *rng, RTSGame *game) {
    (void)k;
    (void)rng;
    (void)game;
}
