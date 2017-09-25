#pragma once
#include <vector>
#include <atomic>
#include <memory>

namespace elf {

using namespace std;

enum GameResult { GAME_NORMAL = 0, GAME_END = 1, GAME_ERROR = 2 };

struct GameBaseOptions {
};

// Any games played by multiple AIs.
template <typename S, typename _AI>
class GameBaseT {
public:
    using GameBase = GameBaseT<S, _AI>;
    using AI = _AI;

    /*
    GameBaseT(const GameBaseOptions &options) : _options(options) {
    }
    */
    GameBaseT(S &s) : _state(s) { }

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

    void MainLoop(const std::atomic_bool *done = nullptr) {
        _state.Init();
        while (! done) {
            _state.PreAct();
            auto t = _state.GetTick();
            for (const auto &bot : _bots) {
                typename AI::Action actions;
                bot->Act(t, &actions, done);
                _state.Forward(actions);
            }
            if (_spectator != nullptr) {
                typename AI::Action actions;
                _spectator->Act(t, &actions, done);
            }

            GameResult res = _state.PostAct();
            if (res == GAME_END) break;

            _state.IncTick();
        }
        // Send message to AIs.
        auto t = _state.GetTick();
        for (const auto &bot : _bots) {
            bot->GameEnd(t);
        }
        if (_spectator != nullptr) _spectator->GameEnd(t);

        _state.Finalize();
    }

    void Reset() {
        _state.Reset();
    }

private:
    S &_state;
    std::vector<unique_ptr<AI>> _bots;
    unique_ptr<AI> _spectator;

    GameBaseOptions _options;
};

}  // namespace elf
