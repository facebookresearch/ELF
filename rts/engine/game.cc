/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#include "game.h"
#include "unit.h"
#include "serializer.h"
#include "cmd.gen.h"
#include <fstream>
#include <chrono>
#include <thread>
#include <string>

using namespace std::chrono;

////////////////////////// RTSStateExtend ////////////////////////////////////
RTSStateExtend::RTSStateExtend(const RTSGameOptions &options)
    : _options(options), _snapshot_to_load(-1), _paused(false), _output_stream_owned(false), _output_stream(nullptr) {
    if (_options.output_file.empty()) {
        _output_stream = _options.output_stream;
        _output_stream_owned = false;
    } else if (_options.output_file == "cout") {
       // cout << "RTSStateExtend: open cout for stdout" << endl;
       _output_stream = &cout;
       _output_stream_owned = false;
    } else {
       // cout << "RTSStateExtend: open " << _options.output_file << " for stdout." << endl;
       _output_stream = new ofstream(_options.output_file);
       _output_stream_owned = true;
    }
}

RTSStateExtend::~RTSStateExtend() {
    if (_output_stream_owned) {
      delete _output_stream;
    }
}

bool RTSStateExtend::change_simulation_speed(float fraction) {
    _options.main_loop_quota /= fraction;
    return true;
}

CmdReturn RTSStateExtend::dispatch_cmds(const UICmd& cmd) {
    switch(cmd.cmd) {
        case UI_SLIDEBAR:
            // UI controls, only works if there is a spectator.
            // cout << "Receive slider bar notification " << cmd.arg2 << endl;
            _snapshot_to_load = RTSState::MoveToTick(_options.snapshots, cmd.arg2 / 100);
            if (_snapshot_to_load >= 0) return CMD_QUEUE_RELOADED;
            break;
        case UI_FASTER_SIMULATION:
            if (change_simulation_speed(1.25)) return CMD_SUCCESS;
            break;
        case UI_SLOWER_SIMULATION:
            if (change_simulation_speed(0.8)) return CMD_SUCCESS;
            break;
        /*
        case UI_CYCLEPLAYER:
            if (_spectator != nullptr) {
                PlayerId id = _spectator->GetId();
                if (id == INVALID) id = 0;
                else id++;
                if (id >= _env.GetNumOfPlayers()) id = INVALID;
                _spectator->SetId(id);
                return CMD_SUCCESS;
            }
            break;
        */
        case TOGGLE_GAME_PAUSE:
            _paused = ! _paused;
            break;
        default:
            cout << "Cmd not handled! " << cmd.PrintInfo() << endl;
            break;
    }
    return CMD_FAILED;
}

bool RTSStateExtend::forward(RTSAction &a) {
    if (! RTSState::forward(a)) return false;

    // Then we also need to send UI commands, if there is any.
    for (const auto &cmd : a.ui_cmds()) {
        dispatch_cmds(cmd);
    }
    return true;
}

bool RTSStateExtend::Init() {
    if (! RTSState::Prepare(_options, _output_stream)) return false;

    if (_output_stream) *_output_stream << "In the main loop " << endl << flush;

    RTSState::SetReplayPrefix(_options.save_replay_prefix);

    _clock.Restart();
    _snapshot_to_load = -1;
    _paused = false;

    const int game_counter = RTSState::env().GetGameCounter();
    _prefix = _options.save_replay_prefix + std::to_string(game_counter);
    if (_output_stream) *_output_stream << "Starting " << _prefix << " Tick: " << RTSState::receiver().GetTick() << endl << flush;

    return true;
}

void RTSStateExtend::PreAct() {
    _time_loop_start = chrono::system_clock::now();
    _clock.SetStartPoint();

    Tick t = RTSState::receiver().GetTick();

    _tick_verbose = (_options.peek_ticks.find(t) != _options.peek_ticks.end());
    _tick_prompt = (_tick_verbose && _output_stream != nullptr);

    RTSState::SetVerbose(_tick_verbose);

    if (_tick_prompt) {
        *_output_stream << "Starting the loop. Tick = " << t << endl << flush;
        // RTSState::receiver().SetPathPlanningVerbose(true);
    }

    if (! _options.snapshot_prefix.empty()) {
        RTSState::SaveSnapshot(_options.snapshot_prefix + "-" + to_string(t) + ".bin", _options.save_with_binary_format);
        _clock.Record("SaveSnapshot");
    }
    if (! _options.snapshot_load_prefix.empty() && _snapshot_to_load >= 0) {
        string filename = _options.snapshot_load_prefix + "-" + to_string(_snapshot_to_load) + ".bin";
        RTSState::LoadSnapshot(filename, _options.save_with_binary_format);
        _snapshot_to_load = -1;
    }

    // Check bots input.
    // Check if we want to peek a specific tick, if so, we print them out.
    if (_tick_prompt) {
        RTSState::env().Visualize();
        if (_output_stream) {
            *_output_stream << RTSState::env().PrintDebugInfo() << flush;
            *_output_stream << "Acting ... " << flush << endl;
        }
    }

    _clock.Record("PreAct");
    /*
       if (_output_stream) {
       uint64_t code = _env.CurrentHashCode();
     *_output_stream << "[" << t << "]: Current hash code: " << hex << code << dec << endl;
     }
     */
}

elf::GameResult RTSStateExtend::PostAct() {
    _clock.Record("Act");

    if (_tick_prompt) *_output_stream << "Forwarding ... " << endl << flush;

    elf::GameResult res = RTSState::PostAct();

    Tick t = RTSState::GetTick();
    const GameEnv &env = RTSState::env();

    if (res != elf::GAME_NORMAL) {
        if (_output_stream) {
            *_output_stream << "[" << t << "][" << env.GetGameCounter() << "] Player " << env.GetWinnerId() << " won!" << endl << flush;
            if (res == elf::GAME_ERROR) {
                *_output_stream << RTSState::receiver().GetGameStats().GetLastError();
            }
        }
    }

    if (_tick_prompt) {
        *_output_stream << " Done with the loop " << endl << flush;
        // RTSState::receiver().SetPathPlanningVerbose(false);
    }

    _clock.Record("PostAct");

    return res;
}

void RTSStateExtend::IncTick() {
    if (! _paused) RTSState::IncTick();

    Tick t = RTSState::GetTick();

    if (_options.tick_prompt_n_step > 0 && t % _options.tick_prompt_n_step == 0) {
        if (_output_stream) *_output_stream << "[" << _prefix << "][" << t << "] Time/tick: " << _clock.Summary() << endl << flush;
        _clock.Restart();
    }

    _clock.Record("IncTick");

    if (_options.main_loop_quota > 0) {
        this_thread::sleep_until(_time_loop_start + chrono::milliseconds(_options.main_loop_quota));
    }
}

/*
PlayerId RTSStateExtend::MainLoop(const std::atomic_bool *done) {
    while (true) {
        if (! _paused) {
            if (!_options.bypass_bot_actions) {
                for (const auto &bot : _bots) {
                    if (_tick_prompt) *_output_stream << "Run bot " << bot->GetId() << endl << flush;
                    bot->Act(_env);
                }
            } else {
                // Replace all actions with replays.
                _cmd_receiver.SendCurrentReplay();
            }
        }
        if (_spectator != nullptr) {
            if (_tick_prompt) *_output_stream << "Run spectator ... " << endl << flush;
            _spectator->Act(_env);
        }

    }

    return _env.GetWinnerId();
}

PlayerId RTSStateExtend::MainLoop(const std::atomic_bool *done) {
  if (! PrepareGame()) return INVALID;

  const int game_counter = _env.GetGameCounter();

  if (_output_stream) *_output_stream << "In the main loop " << endl << flush;

  // move_to_tick(0.5);
  //
  auto default_cmd_dispatch = [&](const UICmd& cmd) { return dispatch_cmds(cmd); };

  // Start the main loop
  MyClock clock;
  clock.Restart();

  _snapshot_to_load = -1;
  _paused = false;

  std::string prefix = _options.save_replay_prefix + std::to_string(game_counter);

  if (_output_stream) *_output_stream << "Starting " << prefix << " Tick: " << _cmd_receiver.GetTick() << endl << flush;

  while (true) {
      auto time_loop_start = chrono::system_clock::now();
      clock.SetStartPoint();

      Tick t = _cmd_receiver.GetTick();
      bool tick_verbose = (_options.peek_ticks.find(t) != _options.peek_ticks.end());
      bool tick_prompt = (tick_verbose && _output_stream != nullptr);

      if (tick_prompt) {
          *_output_stream << "Starting the loop. Tick = " << t << endl << flush;
          _cmd_receiver.SetPathPlanningVerbose(true);
      }

      if (! _options.snapshot_prefix.empty()) {
          save_snapshot(_options.snapshot_prefix + "-" + to_string(t) + ".bin");
          clock.Record("SaveSnapshot");
      }
      if (! _options.snapshot_load_prefix.empty() && _snapshot_to_load >= 0) {
          string filename = _options.snapshot_load_prefix + "-" + to_string(_snapshot_to_load) + ".bin";
          load_snapshot(filename);
          _snapshot_to_load = -1;
      }
      // Check bots input.
      // Check if we want to peek a specific tick, if so, we print them out.
      if (tick_prompt) {
          _env.Visualize();
          if (_output_stream) {
              *_output_stream << _env.PrintDebugInfo() << flush;
              *_output_stream << "Acting ... " << flush << endl;
          }
      }

      // if (_output_stream) {
      //    uint64_t code = _env.CurrentHashCode();
      //    *_output_stream << "[" << t << "]: Current hash code: " << hex << code << dec << endl;
      // }

      if (! _paused) {
          if (!_options.bypass_bot_actions) {
              for (const auto &bot : _bots) {
                  if (tick_prompt) *_output_stream << "Run bot " << bot->GetId() << endl << flush;
                  bot->Act(_env);
              }
          } else {
              // Replace all actions with replays.
              _cmd_receiver.SendCurrentReplay();
          }
      }
      if (_spectator != nullptr) {
          if (tick_prompt) *_output_stream << "Run spectator ... " << endl << flush;
          _spectator->Act(_env);
      }

      clock.Record("Act");

      if (tick_prompt) *_output_stream << "Forwarding ... " << endl << flush;

      _env.Forward(&_cmd_receiver);

      if (tick_prompt) *_output_stream << "Start executing cmds... " << endl << flush;

      _cmd_receiver.ExecuteDurativeCmds(_env, tick_verbose);
      _cmd_receiver.ExecuteImmediateCmds(&_env, tick_verbose);
      _cmd_receiver.ExecuteUICmds(default_cmd_dispatch);
      // cout << "Compute Fow" << endl;
      _env.ComputeFOW();

      clock.Record("Cmd");

      if (tick_prompt) *_output_stream << "Checking winner" << endl << flush;
      PlayerId winner_id = _env.GetGameDef().CheckWinner(_env, _cmd_receiver.GetTick() >= _options.max_tick);
      _env.SetWinnerId(winner_id);

      // Check winning condition
      if (winner_id != INVALID || t >= _options.max_tick || ! _cmd_receiver.GetGameStats().CheckGameSmooth(t, _output_stream)) {
          _env.SetTermination();
          _cmd_receiver.GetGameStats().SetWinner(winner_id);

          if (winner_id != INVALID) {
              _bots[winner_id]->SendComment("Won at " + std::to_string(t));
              if (_output_stream) {
                  *_output_stream << "[" << t << "][" << game_counter << "] Player " << winner_id << " won!" << endl << flush;
              }
          }

          for (const auto &bot : _bots) {
              bot->Act(_env, true);
          }
          if (_spectator != nullptr) {
            _spectator->Act(_env);
          }
          break;
      }

      // If the external signal tells you to stop, stop.
      if (done != nullptr && done->load()) break;
      if (! _paused) _cmd_receiver.IncTick();

      if (tick_prompt) {
          *_output_stream << " Done with the loop " << endl << flush;
          _cmd_receiver.SetPathPlanningVerbose(false);
      }

      clock.Record("Vis");
      if (_options.tick_prompt_n_step > 0 && (t + 1) % _options.tick_prompt_n_step == 0) {
          if (_output_stream) *_output_stream << "[" << prefix << "][" << t << "] Time/tick: " << clock.Summary() << endl << flush;
          clock.Restart();
      }
      if (_options.main_loop_quota > 0) {
          this_thread::sleep_until(time_loop_start + chrono::milliseconds(_options.main_loop_quota));
      }
  }

  // cout << "[" << prefix << "] About to save to rep" << endl;
  if (! _options.save_replay_prefix.empty()) {
      _cmd_receiver.SaveReplay(prefix + ".rep");
  }
  return _env.GetWinnerId();
}
*/

/*
PlayerId RTSStateExtend::Step(int num_ticks, std::string *state) {
    load_from_string(*state);
    for (int i = 0; i < num_ticks; i++) {
        for (const auto &bot : _bots) {
            bot->Act(_env);
        }
        auto default_cmd_dispatch = [&](const UICmd& cmd) { return dispatch_cmds(cmd); };
        _env.Forward(&_cmd_receiver);
        _cmd_receiver.ExecuteDurativeCmds(_env, false);
        _cmd_receiver.ExecuteImmediateCmds(&_env, false);
        _cmd_receiver.ExecuteUICmds(default_cmd_dispatch);
        _env.ComputeFOW();
        PlayerId winner_id = _env.GetGameDef().CheckWinner(_env, _cmd_receiver.GetTick() >= _options.max_tick);
        _env.SetWinnerId(winner_id);
        if (_cmd_receiver.GetTick() >= _options.max_tick) {
            _env.SetTermination();
            return MAX_GAME_LENGTH_EXCEEDED;
        }
        if (winner_id != INVALID)  {
            _env.SetTermination();
            return winner_id;
        }
        _cmd_receiver.IncTick();
    }
    save_to_string(state);
    return INVALID;
}
*/

