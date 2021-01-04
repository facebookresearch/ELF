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
        
        bool isPrint = false; 
        if (game_idx==1)
            isPrint = true;
        
        // Create a game.
        RTSGameOptions op;
        op.seed = (options.seed == 0 ? 0 : options.seed + game_idx);
        op.main_loop_quota = 0;
        op.max_tick = options.max_tick;
        op.save_replay_prefix = (replay_prefix.empty() ? "" : replay_prefix + std::to_string(game_idx) + "-");
        op.snapshot_prefix = "";
        op.output_file = options.output_filename;
        op.cmd_dumper_prefix = options.cmd_dumper_prefix;

        if(isPrint){
            string ss = op.PrintInfo();
            std::cout<<"----------RTSGameOptions----"<<std::endl;
            std::cout<<ss<<std::endl;
        }

        //std::cout << "before running wrapper" << std::endl;

        WrapperCB wrapper(game_idx, context_options, options, comm);
        wrapper.OnGameOptions(&op);  //设置了handicap_level用于gameTD?
  
        //std::cout << "before initializing the game" << std::endl;

        // Note that all the bots created here will be owned by game. 
        // Note that AddBot() will set its receiver. So there is no need to specify it here. 
        RTSStateExtend s(op);    // 初始化 RTSStateExtend 
        RTSGame game(&s);   // 初始化 RTSGame, s = RTSStateExtend

        if(isPrint){
            std::cout<<"----------python options-------"<<endl;
            options.Print();
            
        }        
        //wrapper.OnGameInit(&game, more_params);
        wrapper.OnGameInit(&game, more_params,isPrint); //根据PythonOption初始化AI和玩家并添加到游戏中

        s.SetGlobalStats(&_gstats);  // 设置游戏状态

        unsigned long int seed = (op.seed == 0 ? time(NULL) : op.seed);
        std::mt19937 rng;
        rng.seed(seed);

        int iter = 0;
        if(isPrint)
          std::cout << "Start the main loop" << std::endl;
        while (! signal.IsDone()) {
            wrapper.OnEpisodeStart(iter, &rng, &game);   //将这三个参数转为void类型
            game.MainLoop(&signal.done(),isPrint);
            game.Reset();
            ++ iter;
        }
    }

    std::string PrintInfo() const { return _gstats.PrintInfo(); }
};

