#pragma once
#include <vector>
#include <string>
#include "utils.h"

namespace elf {

template <typename S, typename A>
class AI_T {
public:
    AI_T(const std::string &name) : _name(name), _id(-1) { }

    void SetId(int32_t id) { _id = id; on_set_id(id); }
    void SetState(const S &s) { _state = &s; }
    const std::string &GetName() const { return _name; }

    // Given the current state, perform action and send the action to _a;
    // Return false if this procedure fails.
    virtual bool Act(const std::atomic_bool &done) = 0;
    virtual void Reset() { }

    const A &action() const { return _action;} 

private:
    const std::string _name;
    int32_t _id;

    const S *_state = nullptr;
    A _action;

    virtual void on_set_id(int32_t id) { }
};

template <typename S, typename A, typename AIComm>
class AIWithCommT : public AI_T<S, A> {
public:
    void Init(AIComm *ai_comm) {
        assert(ai_comm);
        _ai_comm = ai_comm;
    }

    void Reset() override { if (_ai_comm != nullptr) _ai_comm->Reset(); }

protected:
    AIComm *_ai_comm = nullptr;
};

enum GameResult { GAME_NORMAL = 0, GAME_END = 1, GAME_ERROR = 2 };

typename <typename S, typename A>
using InitFuncT = function<bool (S *)>; 

typename <typename S, typename A>
using DynamicsFuncT = function<bool (S *, const A &)>; 

typename <typename S>
using SelfDynamicsFuncT = function<GameResult (S *)>; 

struct GameBaseOptions {
};

// Any games played by multiple AIs.
template <typename S, typename A>
class GameBaseT {
public:
    using GameBase = GameBaseT<S, A>;
    using AI = AI_T<S, A>;

    GameBaseT(const GameBaseOptions &options) : _options(options) { 
    }

    void AddBot(AI *bot) {
        if (bot == nullptr) {
            std::cout << "Bot at " << _bots.size() << " cannot be nullptr" << std::endl;
            return;
        }
        bot->SetId(_bots.size());
        bot->SetState(_state);
        _bots.emplace_back(bot);
        _state.OnAddPlayer(_bots.size() - 1);
    }

    void RemoveBot() {
        _state.OnRemovePlayer(_bots.size() - 1);
        _bots.pop_back();
    }

    void AddSpectator(AI *spectator) {
        if (_spectator.get() == nullptr) {
            spectator->SetId(-1);
            spectator->SetState(_state);
            _spectator.reset(spectator);
        }
    }

    const S& GetState() const { return _state; }

    void MainLoop(const std::atomic_bool &done) {
        _state.Init();
        while (! done) {
            _state.PreAct();
            for (const auto &bot : _bots) {
                bot->Act(done);
                const A& action = bot->action();
                _state.Forward(actions);
            }
            if (_spectator != nullptr) _spectator->Act(done);

            GameResult res = _state.PostAct();
            if (res == GAME_END) break;

            _state.IncTick();
        }
        _state.Finalize();
    }

    void Reset() {
        _state.Reset();
        // Send message to AIs.
        for (const auto &bot : _bots) {
            bot->Reset();
        }
    }

private:
    S _state;
    std::vector<unique_ptr<AI>> _bots;
    unique_ptr<AI> _spectator;

    GameBaseOptions _options;
};

}  // namespace elf
