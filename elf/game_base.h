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
        _state->PreAct();
        _act(true, done);
        GameResult res = _state->PostAct();
        _state->IncTick();

        return res;
    }

    void MainLoop(const std::atomic_bool *done = nullptr,bool isPrint = false) {
        if(isPrint)
         std::cout<<"-------MainLoop----------"<<std::endl;
        _state->Init(isPrint);  // 初始化游戏
        // if(isPrint){
        //     std::cout<<"--------Start PleyerInfo"<<std::endl;
        //     std::cout<<_state->env().PrintPlayerInfo()<<std::endl;
        // }
        while (true) {
            if (Step(done) != GAME_NORMAL) break;
            if (done != nullptr && done->load()) break;
        }
        // Send message to AIs.
        _act(false, done);
        _game_end();
        // if(isPrint){
        //     std::cout<<"--------End PleyerInfo"<<std::endl;
        //     std::cout<<_state->env().PrintPlayerInfo()<<std::endl;
        // }
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
        for (const Bot &bot : _bots) {
            if (! check_frameskip || t % bot.frame_skip == 0) {
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
        for (const Bot &bot : _bots) {
            bot.ai->GameEnd();
        }
        if (_spectator != nullptr) {
            std::cout<<"_spectator"<<std::endl;
            _spectator->GameEnd();
        }
    }
};

}  // namespace elf
