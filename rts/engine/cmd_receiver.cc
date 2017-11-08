/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "cmd.h"
#include "cmd_receiver.h"
#include "game_env.h"
#include <initializer_list>

bool CmdReceiver::StartDurativeCmd(CmdDurative *cmd) {
    UnitId id = cmd->id();
    if (id == INVALID) return false;

    FinishDurativeCmd(id);
    _unit_durative_cmd[id] = cmd;
    return true;
}

bool CmdReceiver::FinishDurativeCmd(UnitId id) {
    auto it = _unit_durative_cmd.find(id);
    if (it != _unit_durative_cmd.end()) {
        it->second->SetDone();
        _unit_durative_cmd.erase(it);
        return true;
    }
    else return false;
}

bool CmdReceiver::FinishDurativeCmdIfDone(UnitId id) {
    auto it = _unit_durative_cmd.find(id);
    if (it != _unit_durative_cmd.end() && it->second->IsDone()) {
        _unit_durative_cmd.erase(it);
        return true;
    }
    else return false;
}

bool CmdReceiver::SendCmd(CmdBPtr &&cmd) {
    return SendCmdWithTick(std::move(cmd), _tick);
}

bool CmdReceiver::SendCmdWithTick(CmdBPtr &&cmd, Tick tick) {
    // This id is just used to achieve a total ordering.
    if (cmd.get() == nullptr) {
        throw std::range_error("Error input cmd is nullptr!");
    }
    cmd->set_cmd_id(_cmd_next_id);
    _cmd_next_id ++;
    cmd->set_tick_and_start_tick(tick);

    if (_cmd_dumper != nullptr) *_cmd_dumper << cmd->PrintInfo() << endl;

    // Check wehther we need to save stuff to _cmd_history.
    // For all commands that issued in ExecuteCmd(), we don't need to send them to _cmd_history.
    if (IsSaveToHistory()) _cmd_history.push_back(cmd->clone());

    // Put the command to different queue
    CmdDurative *durative = dynamic_cast<CmdDurative *>(cmd.get());
    if (durative != nullptr) {
        // show_prompt_cond("Receive Durative Cmd", cmd);
        // cout << "Receive Durative Cmd " << cmd->PrintInfo() << endl;
        cmd.release();
        _durative_cmd_queue.push(CmdDPtr(durative));
    } else {
        CmdImmediate *immediate = dynamic_cast<CmdImmediate *>(cmd.get());
        if (immediate != nullptr) {
            // show_prompt_cond("Receive Immediate Cmd", cmd);
            // cout << "Receive Immediate Cmd " << cmd->PrintInfo() << endl;
            cmd.release();
            _immediate_cmd_queue.push(CmdIPtr(immediate));
        } else {
            throw std::range_error("Error! the command is neither durative or immediate! " + cmd->PrintInfo());
        }
    }
    return true;
}

const CmdDurative *CmdReceiver::GetUnitDurativeCmd(UnitId id) const {
    auto it = _unit_durative_cmd.find(id);
    if (it == _unit_durative_cmd.end()) return nullptr;
    else return it->second;
}

bool CmdReceiver::SaveReplay(const string& replay_filename) const {
    // Load the replay_file (which is a action sequence)
    // Each action looks like the following:
    //    Tick, CmdType, UnitId, UnitType, Loc
    //
    serializer::saver saver(false);
    // if (_verbose) cout << "Save replay to " << replay_filename << " #record: " << _cmd_history.size() << endl;
    saver << _cmd_history;
    if (! saver.write_to_file(replay_filename)) return false;

    return true;
}

void CmdReceiver::ExecuteDurativeCmds(const GameEnv &env, bool force_verbose) {
    SetSaveToHistory(false);

    // cout << "Starting ExecutiveDurativeCmds[" << _tick << "]" << endl;

    // Execute durative cmds.
    while (! _durative_cmd_queue.empty()) {
        const CmdDPtr& cmd_ref = _durative_cmd_queue.top();
        // cout << "Top: " << cmd_ref->PrintInfo() << endl;
        if (cmd_ref->tick() > _tick) break;

        show_prompt_cond("ExecuteDurativeCmds", cmd_ref, force_verbose);

        // If the command is done (often set by other preemptive commands, we skip.
        if (cmd_ref->IsDone()) {
            FinishDurativeCmdIfDone(cmd_ref->id());
            _durative_cmd_queue.pop();
            continue;
        }

        // Now finally we deal with the command.
        CmdDPtr cmd = _durative_cmd_queue.pop_top();

        // cout << "Before executing a durative cmd " << cmd->PrintInfo() << endl;

        // Run the cmd.
        cmd->Run(env, this);

        // If the command is not yet done, push it back to the queue.
        if (! cmd->IsDone()) _durative_cmd_queue.push(std::move(cmd));
        else FinishDurativeCmd(cmd->id());
    }

    // cout << "Ending ExecutiveDurativeCmds[" << _tick << "]" << endl;
    SetSaveToHistory(true);
}

void CmdReceiver::ExecuteImmediateCmds(GameEnv *env, bool force_verbose) {
    SetSaveToHistory(false);

    // cout << "Starting ExecutiveImmediateCmds[" << _tick << "]" << endl;

    // Execute immediate cmds, which will change the game state.
    while (! _immediate_cmd_queue.empty()) {
        const CmdIPtr &cmd_ref = _immediate_cmd_queue.top();
        // cout << "Top: " << cmd_ref->PrintInfo() << endl;
        if (cmd_ref->tick() > _tick) break;

        CmdIPtr cmd = _immediate_cmd_queue.pop_top();

        show_prompt_cond("ExecuteImmediateCmds", cmd, force_verbose);

        cmd->Run(env, this);
    }

    // cout << "Ending ExecutiveImmediateCmds[" << _tick << "]" << endl;
    SetSaveToHistory(true);
}

vector<CmdDurative*> CmdReceiver::GetHistoryAtCurrentTick() const {
    vector<CmdDurative*> res;
    for (int i = _cmd_history.size() - 1; i >= 0; i--) {
        const auto &cmd = _cmd_history[i];
        if (cmd->tick() < _tick) break;
        CmdDurative *durative = dynamic_cast<CmdDurative *>(cmd.get());
        if (durative != nullptr) {
            res.push_back(durative);
        }
    }
    return res;

}


void CmdReceiver::SaveCmdReceiver(serializer::saver &saver) const {
    // Do not save/load _loaded_replay, as well as command history.
    saver << _tick << _immediate_cmd_queue << _durative_cmd_queue << _verbose_player_id << _verbose_choice;
}

void CmdReceiver::LoadCmdReceiver(serializer::loader &loader) {
    loader >> _tick >> _immediate_cmd_queue >> _durative_cmd_queue >> _verbose_player_id >> _verbose_choice;

    // Set the failed_moves.
    _stats.SetTick(_tick);

    // load durative cmd queue. Note that the priority queue is opaque so we need some hacks.
    int size = _durative_cmd_queue.size();
    const CmdDPtr *c = &(_durative_cmd_queue.top());
    _unit_durative_cmd.clear();
    for (int i = 0; i < size; ++i) {
        const CmdDPtr &curr = c[i];
        if (! curr->IsDone()) _unit_durative_cmd.insert(make_pair(curr->id(), curr.get()));
    }
}

void CmdReceiver::SetCmdDumper(const string& cmd_dumper_filename) {
    // Set the command dumper if there is any file specified.
    _cmd_dumper.reset(new ofstream(cmd_dumper_filename));
}
