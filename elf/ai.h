#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <atomic>
#include "utils.h"

namespace elf {

using Tick = int;

using namespace std;

template <typename S, typename A>
class AI_T {
public:
    using Action = A;
    using State = S;

    AI_T() : _name("noname"), _frame_skip(1) { }
    AI_T(const std::string &name, int frame_skip) : _name(name), _id(-1), _frame_skip(frame_skip) { }

    void SetId(int id) {
        _id = id;
        on_set_id();
        // cout << "SetId: " << id << endl;
    }

    void SetState(const S &s) {
        _state = &s;
        on_set_state();
        // cout << "SetState " << endl;
    }

    const std::string &name() const { return _name; }
    int id() const { return _id; }
    const S& s() const {
        if (_state == nullptr) {
            cout << "AI_T::_state is null! " << endl;
            throw std::range_error("AI_T::_state is null!");
        }
        return *_state;
    }
    const S* s_ptr() const { return _state; }

    // Given the current state, perform action and send the action to _a;
    // Return false if this procedure fails.
    bool Act(Tick t, A *a, const std::atomic_bool *done) {
        if (t % _frame_skip == 0) return on_act(t, a, done);
        else return false;
    }

    virtual bool GameEnd(Tick) { return true; }

protected:
    const std::string _name;
    int _id;

    // Run on_act() every _frame_skip
    int _frame_skip;

    const S *_state = nullptr;

    virtual void on_set_id() { }
    virtual void on_set_state() { }
    virtual bool on_act(Tick, A *, const std::atomic_bool *) { return true; }
};

template <typename S, typename A, typename AIComm>
class AIWithCommT : public AI_T<S, A> {
public:
    using Data = typename AIComm::Data;

    AIWithCommT() { }
    AIWithCommT(const std::string &name, int frame_skip) : AI_T<S, A>(name, frame_skip) { }

    void InitAIComm(AIComm *ai_comm) {
        assert(ai_comm);
        _ai_comm = ai_comm;
    }

    // Get called when we start a new game.
    bool GameEnd(Tick t) override {
        if (_ai_comm == nullptr) return false;

        // Send final message.
        on_act(t, nullptr, nullptr);

        // Restart _ai_comm.
        _ai_comm->Restart();
        return true;
    }

protected:
    AIComm *_ai_comm = nullptr;

    bool on_act(Tick, A *a, const std::atomic_bool *) override {
        _ai_comm->Prepare();
        Data *data = &_ai_comm->info().data;
        extract(data);
        if (! _ai_comm->SendDataWaitReply()) return false;

        // Then deal with the response, if a != nullptr
        if (a != nullptr) handle_response(_ai_comm->info().data, a);
        return true;
    }

    // Extract and save to data.
    virtual void extract(Data *data) = 0;
    virtual bool handle_response(const Data &data, A *a) = 0;
};

}  // namespace elf
