/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef _CMD_RECEIVER_H_
#define _CMD_RECEIVER_H_

#include "cmd.h"
#include "ui_cmd.h"
#include "game_stats.h"

#include "pq_extend.h"
#include <map>
#include <functional>
// #include "Selene.h"

// receive command and record them in the history.
class CmdReceiver;
class GameEnv;

// A command is either durative or immediate.
// For immediate command, it has immediate effects.
// For durative command, it will send downstream sub-commands
//
custom_enum(CmdReturn, CMD_SUCCESS = 0, CMD_FAILED, CMD_QUEUE_RELOADED);

typedef int VerboseChoice;
#define CR_NO_VERBOSE 0
#define CR_DURATIVE 1
#define CR_IMMEDIATE 2
#define CR_DERIVED 4
#define CR_NODERIVED 8
#define CR_ALL ( CR_DURATIVE | CR_IMMEDIATE | CR_DERIVED | CR_NODERIVED )

// receive command and record them in the history.
// The cmds are ordered so that the lowest level is executed first.
class CmdReceiver {
private:
    Tick _tick;
    int _cmd_next_id;

    GameStats _stats;

    p_queue<CmdIPtr> _immediate_cmd_queue;
    p_queue<CmdDPtr> _durative_cmd_queue;
    std::queue<UICmd> _ui_cmd_queue;

    vector<CmdBPtr> _cmd_history;

    // Idx for the next replay to send to the queue.
    unsigned int _next_replay_idx;
    vector<CmdBPtr> _loaded_replay;

    // Record current state of each unit. Note that this pointer does not own anything.
    // When the command is destroyed, we should manually delete the entry as well.
    map<UnitId, CmdDurative *> _unit_durative_cmd;

    // Use to dump SendCmd.
    unique_ptr<ostream> _cmd_dumper;

    // Whether we save the current issued command to the history buffer.
    bool _save_to_history;

    // For player id, talk a bit more.
    // id == INVALID means verbose to all players.
    int _verbose_player_id;
    VerboseChoice _verbose_choice;
    bool _path_planning_verbose;
    bool _use_cmd_comment;

    template <typename CmdType>
    bool show_prompt_cond(const string &prompt, const unique_ptr<CmdType> &cmd, bool force_verbose = false) const {
        if (force_verbose) {
            cout << prompt << " " << cmd->PrintInfo() << endl;
            return true;
        }
        /* if (_verbose_choice == CR_NO_VERBOSE) return false;
           if (_verbose_player_id == INVALID || player_id == _verbose_player_id) {
           if ( (_verbose_choice & CR_DURATIVE) && ! _cmd_template.IsDurativeCmd(cmd.cmd)) return false;
           if ( (_verbose_choice & CR_IMMEDIATE) && ! _cmd_template.IsImmediateCmd(cmd.cmd)) return false;
           cout << "[" << player_id << "] " << prompt << ": " << cmd << endl;
           return true;
           }
        */
        else return false;
    }

public:
    CmdReceiver()
        : _tick(0), _cmd_next_id(0), _next_replay_idx(-1),
          _cmd_dumper(nullptr), _save_to_history(true),
          _verbose_player_id(INVALID), _verbose_choice(CR_NO_VERBOSE), _path_planning_verbose(false), _use_cmd_comment(false)  {
    }

    const GameStats &GetGameStats() const { return _stats; }
    GameStats &GetGameStats() { return _stats; }

    Tick GetTick() const { return _tick; }
    Tick GetNextTick() const { return _tick + 1; }
    inline void IncTick() { _tick ++; _stats.IncTick(); }
    inline void ResetTick() { _tick = 0; _stats.Reset(); }

    void SetCmdDumper(const string &cmd_dumper_filename);

    void SetUseCmdComment(bool use_comment) { _use_cmd_comment = use_comment; }
    bool GetUseCmdComment() const { return _use_cmd_comment; }
    void SetPathPlanningVerbose(bool verbose) { _path_planning_verbose = verbose; }
    bool GetPathPlanningVerbose() const { return _path_planning_verbose; }

    void SetVerbose(VerboseChoice choice, PlayerId player_id) {
        _verbose_choice = choice;
        _verbose_player_id = player_id;
    }
    void ClearCmd() {
        while (! _immediate_cmd_queue.empty()) _immediate_cmd_queue.pop();
        while (! _durative_cmd_queue.empty()) _durative_cmd_queue.pop();
        while (! _ui_cmd_queue.empty()) _ui_cmd_queue.pop();
        _cmd_history.clear();
        _unit_durative_cmd.clear();
        _cmd_next_id = 0;
    }

    // Send command with the current tick.
    bool SendCmd(CmdBPtr &&cmd);
    // Send command with a specific tick.
    bool SendCmdWithTick(CmdBPtr &&cmd, Tick tick);
    bool SendCmd(UICmd &&cmd);

    // Set this to be true to prevent any command to be recorded in the history.
    void SetSaveToHistory(bool v) { _save_to_history = v; }
    bool IsSaveToHistory() const { return _save_to_history; }

    // Start a durative cmd specified by the pointer.
    bool StartDurativeCmd(CmdDurative *);

    // Finish the durative command for unit id.
    bool FinishDurativeCmd(UnitId id);
    bool FinishDurativeCmdIfDone(UnitId id);

    const CmdDurative *GetUnitDurativeCmd(UnitId id) const;
    int GetLoadedReplaySize() const { return _loaded_replay.size(); }
    int GetLoadedReplayLastTick() const { return _loaded_replay.back()->tick(); }
    vector<CmdDurative*> GetHistoryAtCurrentTick() const;

    // Save and load Replay from a file
    bool LoadReplay(const string& replay_filename);
    bool SaveReplay(const string& replay_filename) const;

    // Execute Durative Commands. This will not change the game environment.
    void ExecuteDurativeCmds(const GameEnv &env, bool force_verbose);
    // Execute Immediate Commands. This will change the game environment.
    void ExecuteImmediateCmds(GameEnv *env, bool force_verbose);
    // Execute UI Commands from gui.
    void ExecuteUICmds(function<void (const UICmd&)> default_f);

    // Send replay from this tick.
    void SendCurrentReplay();
    void AlignReplayIdx();

    // CmdReceiver has its specialized Save and Load function.
    // No SERIALIZER(...) is needed.
    void SaveCmdReceiver(serializer::saver &saver) const;
    void LoadCmdReceiver(serializer::loader &loader);

    ~CmdReceiver() { }
};

#endif
