#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <atomic>
#include "utils.h"

namespace elf {

using namespace std;

template <typename S, typename A>
class AI_T {
public:
    using Action = A;
    using State = S;

    AI_T() : _name("noname") { }
    AI_T(const std::string &name) : _name(name), _id(-1) { }

    void SetId(int id) {
        _id = id;
        on_set_id();
        // cout << "SetId: " << id << endl;
    }

    const std::string &name() const { return _name; }
    int id() const { return _id; }

    // Given the current state, perform action and send the action to _a;
    // Return false if this procedure fails.
    virtual bool Act(const S &, A *, const std::atomic_bool *) { return true; }

    virtual bool GameEnd() { return true; }

    virtual ~AI_T() { }

private:
    const std::string _name;
    int _id;

protected:
    virtual void on_set_id() { }
};

template <typename S, typename A, typename AIComm>
class AIWithCommT : public AI_T<S, A> {
public:
    using AI = AI_T<S, A>;
    using Data = typename AIComm::Data;

    AIWithCommT() { }
    AIWithCommT(const std::string &name) : AI(name) { }

    void InitAIComm(AIComm *ai_comm) {
        assert(ai_comm);
        _ai_comm = ai_comm;
        on_set_ai_comm();
    }

    bool Act(const S &s, A *a, const std::atomic_bool *done) override {
        assert(_ai_comm);
        before_act(s, done);
        _ai_comm->Prepare();
        Data *data = &_ai_comm->info().data;
        extract(s, data);
        if (! _ai_comm->SendDataWaitReply()) return false;

        // Then deal with the response, if a != nullptr
        if (a != nullptr) handle_response(s, _ai_comm->info().data, a);
        return true;
    }

    const Data& data() const {
        assert(_ai_comm);
        return _ai_comm->info().data;
    }
    const AIComm *ai_comm() const { return _ai_comm; }
    AIComm *ai_comm() { return _ai_comm; }

    // Get called when we start a new game.
    bool GameEnd() override {
        if (_ai_comm == nullptr) return false;

        // Restart _ai_comm.
        _ai_comm->Restart();
        return true;
    }

protected:
    AIComm *_ai_comm = nullptr;

    // Extract and save to data.
    virtual void extract(const S &s, Data *data) = 0;
    virtual bool handle_response(const S &s, const Data &data, A *a) = 0;
    virtual void on_set_ai_comm() { }
    virtual void before_act(const S &, const std::atomic_bool *) { }
};

template <typename S, typename A>
class AIHoldStateT {
public:
    using Action = A;
    using State = S;

    AIHoldStateT() : _name("noname") { }
    AIHoldStateT(const std::string &name) : _name(name), _id(-1) { }

    void SetId(int id) {
        _id = id;
        on_set_id();
        // cout << "SetId: " << id << endl;
    }

    const std::string &name() const { return _name; }
    int id() const { return _id; }

    const S& s() const { return s_; }

    // Given the current state, perform action and send the action to _a;
    // Return false if this procedure fails.
    virtual bool Act(A *, const std::atomic_bool *) { return true; }

protected:
    const std::string _name;
    int _id;

    S s_;

    virtual void on_set_id() { }
    S &s() { return s_; }
};

template <typename S, typename A, typename AIComm>
class AIHoldStateWithCommT : public AIHoldStateT<S, A> {
public:
    using AI = AI_T<S, A>;
    using Data = typename AIComm::Data;

    AIHoldStateWithCommT() { }
    AIHoldStateWithCommT(const std::string &name) : AI(name) { }

    void InitAIComm(AIComm *ai_comm) {
        assert(ai_comm);
        _ai_comm = ai_comm;
        on_set_ai_comm();
    }

    bool Act(A *a, const std::atomic_bool *done) override {
        assert(_ai_comm);
        before_act(done);
        _ai_comm->Prepare();
        Data *data = &_ai_comm->info().data;
        extract(data);
        if (! _ai_comm->SendDataWaitReply()) return false;

        // Then deal with the response, if a != nullptr
        if (a != nullptr) handle_response(_ai_comm->info().data, a);
        return true;
    }

    const Data& data() const {
        assert(_ai_comm);
        return _ai_comm->info().data;
    }
    const AIComm *ai_comm() const { return _ai_comm; }
    AIComm *ai_comm() { return _ai_comm; }

protected:
    AIComm *_ai_comm = nullptr;

    // Extract and save to data.
    virtual void extract(Data *data) = 0;
    virtual bool handle_response(const Data &data, A *a) = 0;
    virtual void on_set_ai_comm() { }
    virtual void before_act(const std::atomic_bool *) { }
};

}  // namespace elf
