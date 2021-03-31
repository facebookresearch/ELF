/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

#include "game.h"
#include "../elf/game_base.h"
#include "../elf/signal.h"
#include "../elf/python_options_utils_cpp.h"

template <typename WrapperCB, typename Comm, typename PythonOptions, typename AI>
class WrapperT {
public:
    using Wrapper = WrapperT<WrapperCB, Comm, PythonOptions, AI>;
    using RTSGame = elf::GameBaseT<RTSState, AI>;

private:
    GlobalStats _gstats;

public:
    WrapperT() {
    }

    void thread_main(int game_idx, const ContextOptions &context_options,
            const PythonOptions &options, const elf::Signal &signal,
            const std::map<std::string, int> *more_params, Comm *comm) {
        const string& replay_prefix = options.save_replay_prefix;

        // Create a game.
        // bool isPrint = false;
        // if(game_idx == 1){
        //     isPrint = true;
        // }
        RTSGameOptions op;
        op.seed = (options.seed == 0 ? 0 : options.seed + game_idx);
        op.main_loop_quota = 0;
        op.max_tick = options.max_tick;
        op.save_replay_prefix = (replay_prefix.empty() ? "" : replay_prefix + std::to_string(game_idx) + "-");
        op.snapshot_prefix = "";
        op.output_file = options.output_filename;
        op.cmd_dumper_prefix = options.cmd_dumper_prefix;
        
        
        // if(isPrint){
        //     std::cout << "before running wrapper" << std::endl;
        // }

        WrapperCB wrapper(game_idx, context_options, options, comm);
        wrapper.OnGameOptions(&op);

        // if(isPrint){
        //    std::cout << "RTSStateExtend" << std::endl;
        // }
        
        // if(isPrint){
        //      std::cout<<"RTSGameOptions"<<std::endl;
        //      std::cout<<op.PrintInfo()<<std::endl;
        // }

        

        // Note that all the bots created here will be owned by game.
        // Note that AddBot() will set its receiver. So there is no need to specify it here.
        RTSStateExtend s(op);

        // if(isPrint){
        //    std::cout << "RTSGame" << std::endl;
        // }

        RTSGame game(&s);
        
        wrapper.OnGameInit(&game, more_params);
        // if(isPrint){
        //    std::cout << "before SetGlobalStats " << std::endl;
        // }

        s.SetGlobalStats(&_gstats);

        unsigned long int seed = (op.seed == 0 ? time(NULL) : op.seed);
        std::mt19937 rng;
        rng.seed(seed);

        int iter = 0;

        // if(isPrint){
        //    std::cout << "Start the main loop" << std::endl;
        // }
        
        while (! signal.IsDone()) {
            wrapper.OnEpisodeStart(iter, &rng, &game);
            
            game.MainLoop(&signal.done());
            game.Reset();
            ++ iter;
        }
    }

    std::string PrintInfo() const { return _gstats.PrintInfo(); }
};

