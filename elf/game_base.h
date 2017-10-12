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
template <typename S, typename _AI>
class GameBaseT {
public:
    using GameBase = GameBaseT<S, _AI>;
    using AI = _AI;

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

    void AddSpectator(AI *spectator) {
        if (_spectator.get() == nullptr) {
            spectator->SetId(-1);
            _spectator.reset(spectator);
        }
    }

    const S& GetState() const { return *_state; }
    S &GetState() { return *_state; }
    void SetState(S *s) { _state = s; }

    GameResult Step(const std::atomic_bool *done = nullptr) {
        _state->PreAct();
        auto t = _state->GetTick();
        for (const Bot &bot : _bots) {
            if (t % bot.frame_skip == 0) {
                typename AI::Action actions;
                bot.ai->Act(*_state, &actions, done);
                _state->forward(actions);
            }
        }
        if (_spectator != nullptr) {
            typename AI::Action actions;
            _spectator->Act(*_state, &actions, done);
            _state->forward(actions);
        }

        GameResult res = _state->PostAct();
        _state->IncTick();

        return res;
    }

    void MainLoop(const std::atomic_bool *done = nullptr) {
        _state->Init();
        while (true) {
            if (Step(done) != GAME_NORMAL) break;
            if (done != nullptr && done->load()) break;
        }
        // Send message to AIs.
        for (const Bot &bot : _bots) {
            bot.ai->GameEnd(*_state);
        }
        if (_spectator != nullptr) _spectator->GameEnd(*_state);
        _state->Finalize();
    }

    void Reset() {
        _state->Reset();
    }

private:
    S *_state;
    std::vector<Bot> _bots;
    unique_ptr<AI> _spectator;

    GameBaseOptions _options;
};

}  // namespace elf
