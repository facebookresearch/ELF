/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#pragma once

#include "../../elf/comm_template.h"

#include "cmd_receiver.h"
#include "rule_actor.h"
#include <atomic>
#include <chrono>
#include <algorithm>

class AI {
protected:
    PlayerId _player_id;
    CmdReceiver *_receiver;

    // Run Act() every _frame_skip
    int _frame_skip;

    virtual bool send_cmd_anyway() const { return false; }
    virtual bool on_act(const GameEnv&) { return true; }
    virtual void on_set_id(PlayerId) { }
    virtual void on_set_cmd_receiver(CmdReceiver*) { }
    virtual RuleActor *rule_actor() { return nullptr; }

    bool add_command(CmdBPtr &&cmd) {
        // cout << "Receive cmd " << cmd->PrintInfo() << endl;
        if (_receiver == nullptr) return false;
        if ( send_cmd_anyway() || (cmd->id() != INVALID && Player::ExtractPlayerId(cmd->id()) == _player_id) ) {
            // cout << "About to push cmd: " << cmd.PrintInfo() << endl;
            _receiver->SendCmd(std::move(cmd));
            return true;
        }
        return false;
    }
    void actual_send_cmds(const GameEnv &env, AssignedCmds &assigned_cmds);
    bool gather_decide(const GameEnv &env, std::function<bool (const GameEnv&, string *, AssignedCmds *)> func);

public:
    AI() : _player_id(INVALID), _receiver(nullptr), _frame_skip(1) { }
    AI(PlayerId player_id, int frameskip, CmdReceiver *receiver) : _player_id(player_id), _receiver(receiver), _frame_skip(frameskip) { }
    virtual ~AI() {}
    PlayerId GetId() const { return _player_id; }

    void SetId(PlayerId id) {
        on_set_id(id);
        _player_id = id;
        if (rule_actor() != nullptr) rule_actor()->SetPlayerId(id);
    }

    void SetCmdReceiver(CmdReceiver *receiver) {
        on_set_cmd_receiver(receiver);
        _receiver = receiver;
        if (rule_actor() != nullptr) rule_actor()->SetReceiver(receiver);
    }

    void SendComment(const string&);

    // Get called when the bot is allowed to act.
    virtual bool Act(const GameEnv &env, bool must_act = false) {
        (void)env;
        (void)must_act;
        return true;
    }

    // Get called when we start a new game.
    virtual void Reset() { }

    // Used to plot the feature extracted by the AI.
    virtual string PlotStructuredState(const GameEnv &env) const {
        (void)env;
        return "";
    }

    virtual bool NeedAct(Tick tick) const { return tick % _frame_skip == 0; }
    virtual void SetFactory(std::function<AI* (int)> factory) { (void)factory; }

    // Get internal state.
    // [TODO]: Not a good interface..
    virtual vector<int> GetState() const { return vector<int>(); }

    // Virtual functions.
    // This is for visualization.
    virtual bool IsUnitSelected(UnitId) const { return false; }
    virtual vector<int> GetAllSelectedUnits() const { return vector<int>(); }

    SERIALIZER_BASE(AI, _player_id);
    SERIALIZER_ANCHOR(AI);
};

// A simple AI with AIComm
template <typename AIComm>
class AIWithComm : public AI {
public:
    using Data = typename AIComm::Data;

protected:
    std::unique_ptr<AIComm> _ai_comm;
    std::function<AI* (int)> _factory;

    vector<int> _state;

    // This function is called by Act.
    // In specific situations (e.g., MCTS), it is used separately to get the value of the current situation.
    bool send_data_wait_reply(const GameEnv& env);

    string plot_structured_state(const Data &data) const;

    virtual void on_save_data(Data *data) { (void)data; }

    virtual bool need_structured_state(Tick) const { return _ai_comm != nullptr; }
    virtual void save_structured_state(const GameEnv &env, Data *data) const {
        (void)env;
        (void)data;
    }

public:
    AIWithComm() { }
    AIWithComm(PlayerId id, int frame_skip, CmdReceiver *receiver, AIComm *ai_comm = nullptr)
        : AI(id, frame_skip, receiver), _ai_comm(ai_comm) {
    }
    bool Act(const GameEnv &env, bool must_act = false) override;

    // Get called when we start a new game.
    void Reset() override { if (_ai_comm != nullptr) _ai_comm->Restart(); }

    // Save game state to communicate with python wrapper.
    string PlotStructuredState(const GameEnv &env) const override;
    void SetFactory(std::function<AI* (int)> factory) override { _factory = factory; }

    void SetState(vector<int> state) { _state = state; }
    vector<int> GetState() const override { return _state; }
};

///////////////////////// AIWithComm //////////////////////
template <typename AIComm>
bool AIWithComm<AIComm>::Act(const GameEnv &env, bool must_act) {
    Tick t = _receiver->GetTick();
    if (! must_act && ! NeedAct(t)) return false;

    bool perform_action = true;

    if (need_structured_state(t)) {
        // send structured data for trainable decision making.
        // For trainable bot, compute_structured_state sends the state to the model,
        // and the model makes a decision, which drives Act(). We call GetAction to wait until
        // an action (or information relevant to the action) is returned.
        // Save structure in the bot.
        perform_action = send_data_wait_reply(env);
    }

    // Finally act.
    if (perform_action) return on_act(env);
    else return false;
}

template <typename AIComm>
bool AIWithComm<AIComm>::send_data_wait_reply(const GameEnv& env) {
    _ai_comm->Prepare();
    Data *data = &_ai_comm->info().data;
    save_structured_state(env, data);
    on_save_data(data);
    // cout << PlotStructuredState(*_ai_comm->GetData()) << endl;
    return _ai_comm->SendDataWaitReply();
}

template <typename AIComm>
string AIWithComm<AIComm>::plot_structured_state(const Data &data) const {
    std::stringstream ss;
    ss << "BotId: " << _player_id << endl;
    // TODO: Need to implement.
    (void)data;
    ss << "Not implemented " << endl;
    return ss.str();
}

template <typename AIComm>
string AIWithComm<AIComm>::PlotStructuredState(const GameEnv &env) const {
    Data data;
    save_structured_state(env, &data);
    // Then we plot it.
    return plot_structured_state(data);
}
