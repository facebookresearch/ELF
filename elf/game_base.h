#pragma once
#include <vector>
#include <atomic>
#include <memory>

namespace elf {

using namespace std;

enum GameResult { GAME_NORMAL = 0, GAME_END = 1, GAME_ERROR = 2 };

using Tick = int;

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

    void AddBot(AI *bot, Tick frame_skip) {
        if (bot == nullptr) {
            std::cout << "Bot at " << _bots.size() << " cannot be nullptr" << std::endl;
            return;
        }
        bot->SetId(_bots.size());
        _bots.emplace_back(bot);
        _frame_skips.emplace_back(frame_skip);
        _state.OnAddPlayer(_bots.size() - 1);
    }

    void RemoveBot() {
        _state.OnRemovePlayer(_bots.size() - 1);
        _bots.pop_back();
        _frame_skips.pop_back();
    }

    void AddSpectator(AI *spectator) {
        if (_spectator.get() == nullptr) {
            spectator->SetId(-1);
            _spectator.reset(spectator);
        }
    }

    const S& GetState() const { return _state; }

    void MainLoop(const std::atomic_bool *done = nullptr) {
        _state.Init();
        while (true) {
            _state.PreAct();
            auto t = _state.GetTick();
            for (size_t i = 0; i < _bots.size(); ++i) {
                AI *bot = _bots[i].get();
                if (t % _frame_skips[i] == 0) { 
                    typename AI::Action actions;
                    bot->Act(_state, &actions, done);
                    _state.forward(actions);
                }
            }
            if (_spectator != nullptr) {
                typename AI::Action actions;
                _spectator->Act(_state, &actions, done);
                _state.forward(actions);
            }

            GameResult res = _state.PostAct();
            if (res != GAME_NORMAL) break;
            if (done != nullptr && done->load()) break;

            _state.IncTick();
        }
        // Send message to AIs.
        auto t = _state.GetTick();
        for (const auto &bot : _bots) {
            bot->GameEnd(_state);
        }
        if (_spectator != nullptr) _spectator->GameEnd(_state);

        _state.Finalize();
    }

    void Reset() {
        _state.Reset();
    }

private:
    S &_state;
    std::vector<unique_ptr<AI>> _bots;
    std::vector<Tick> _frame_skips;
    unique_ptr<AI> _spectator;

    GameBaseOptions _options;
};

}  // namespace elf
