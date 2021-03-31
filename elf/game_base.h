/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once
#include <vector>
#include <atomic>
#include <memory>
#include <typeinfo>

namespace elf {

using namespace std;

enum GameResult { GAME_NORMAL = 0, GAME_END = 1, GAME_ERROR = 2 };

using Tick = int;

struct GameBaseOptions {
};

// Any games played by multiple AIs.
template <typename S, typename _AI, typename _Spectator = _AI>
class GameBaseT {
public:
    using GameBase = GameBaseT<S, _AI, _Spectator>;
    using AI = _AI;
    using Spectator = _Spectator;

    struct Bot {
        unique_ptr<AI> ai;
        Tick frame_skip = 1;

        Bot(AI *ai, Tick fs)
            : ai(ai), frame_skip(fs) {
        }
    };

    /*
    GameBaseT(const GameBaseOptions &options) : _options(options) {
    }
    */
    GameBaseT(S *s = nullptr) : _state(s) { }

    void AddBot(AI *bot, Tick frame_skip) {
        if (bot == nullptr) {
            std::cout << "Bot at " << _bots.size() << " cannot be nullptr" << std::endl;
            return;
        }
        
        
        bot->SetId(_bots.size());
        // std::cout<<"====Bot Info ==="<<std::endl;
        // bot->Print();
        _bots.emplace_back(bot, frame_skip);
    }

    void RemoveBot() {
        _bots.pop_back();
    }

    void AddSpectator(Spectator *spectator) {
        if (_spectator.get() == nullptr) {
            _spectator.reset(spectator);
        }
    }

    const S& GetState() const { return *_state; }
    S &GetState() { return *_state; }
    void SetState(S *s) { _state = s; }

    GameResult Step(const std::atomic_bool *done = nullptr) {
        //printf("step\n");
        //clock_t startTime,endTime;
       // startTime = clock();
        //cout<<"----Before PreAct() Tick: "<< _state->receiver().GetTick()<<endl;   
        _state->PreAct();
       // cout<<"----After PreAct() Tick: "<< _state->receiver().GetTick()<<endl;  
        _act(true, done);
        //cout<<"----After _act() Tick: "<< _state->receiver().GetTick()<<endl; 
        GameResult res = _state->PostAct();
        //cout<<"res: "<<res<<endl;
        //cout<<"----After PostAct() Tick: "<< _state->receiver().GetTick()<<endl; 
        _state->IncTick();

        //endTime = clock();
        //std::cout << "The run time between "<<start_tick<<" and "<< _state->receiver().GetTick()<<" is: " <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s" << std::endl;
        //if(res!=GAME_NORMAL) printf("NotNormal %d\n",res);
        return res;
    }

    void MainLoop(const std::atomic_bool *done = nullptr) {
        // if(isPrint)
        //  cout<<"-------MainLoop----------"<<endl;
       // printf("=======Game Start=======\n");
        _state->Init();  // 初始化游戏
        
        // 游戏循环
        while (true) {
            if (Step(done) != GAME_NORMAL) break;   // 如果游戏异常，结束循环
            if (done != nullptr && done->load()) break; 
        } 
        // Send message to AIs.
        _act(false, done);
        _game_end();
        
        _state->Finalize();
        
    }

    void Reset() {
        _state->Reset();
    }

private:
    S *_state;
    std::vector<Bot> _bots;
    unique_ptr<Spectator> _spectator;

    GameBaseOptions _options;

    void _act(bool check_frameskip, const std::atomic_bool *done) {
        
        auto t = _state->GetTick();
        // 更新玩家的奖励
        for (const Bot &bot : _bots) {
            if (! check_frameskip || t % bot.frame_skip == 0) {
                //std::cout<<"Bot Act info"<<std::endl;
              //  bot.ai->Print();
                
                typename AI::Action actions;
                bot.ai->Act(*_state, &actions, done);
                _state->forward(actions);
            }
        }
        if (_spectator != nullptr) {
            typename Spectator::Action actions;
            _spectator->Act(*_state, &actions, done);
            _state->forward(actions);
        }
    }

    void _game_end() {
       // printf("=====Game End=======\n");
        for (const Bot &bot : _bots) {
            bot.ai->GameEnd();
        }
        if (_spectator != nullptr) {
            //std::cout<<"_spectator"<<std::endl;
            _spectator->GameEnd();
        }
    }
};

}  // namespace elf
