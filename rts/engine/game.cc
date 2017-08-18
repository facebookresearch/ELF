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

////////////////////////// RTSGame ////////////////////////////////////
RTSGame::RTSGame(const RTSGameOptions &options)
    : _options(options), _cmd_receiver(), _snapshot_to_load(-1), _paused(false), _output_stream_owned(false), _output_stream(nullptr) {
    if (_options.output_file.empty()) {
        _output_stream = _options.output_stream;
        _output_stream_owned = false;
    } else if (_options.output_file == "cout") {
       // cout << "RTSGame: open cout for stdout" << endl;
       _output_stream = &cout;
       _output_stream_owned = false;
    } else {
       // cout << "RTSGame: open " << _options.output_file << " for stdout." << endl;
       _output_stream = new ofstream(_options.output_file);
       _output_stream_owned = true;
    }

    _bots.clear();
    _env.InitGameDef();
    _env.ClearAllPlayers();
}

RTSGame::~RTSGame() {
    if (_output_stream_owned) {
      delete _output_stream;
    }
}

void RTSGame::AddBot(AI *bot) {
    bot->SetId(_bots.size());
    bot->SetCmdReceiver(&_cmd_receiver);
    _bots.push_back(unique_ptr<AI>(bot));
    _env.AddPlayer(PV_KNOW_ALL);
}

void RTSGame::RemoveBot() {
    _bots.pop_back();
    _env.RemovePlayer();
}

void RTSGame::AddSpectator(AI *spectator) {
    if (_spectator.get() == nullptr) {
        _spectator.reset(spectator);
    }
}

// 0.0 - 1.0
bool RTSGame::move_to_tick(float percent) {
    // Move to a specific tick.
    int num_replay_entry = _cmd_receiver.GetLoadedReplaySize();
    cout << "#replay = " << num_replay_entry << endl;
    cout << "snapshot_load_prefix = " << _options.snapshot_load_prefix << endl;
    cout << "#snapshot = " << _options.snapshots.size() << endl;

    if (num_replay_entry == 0 || _options.snapshot_load_prefix.empty()) {
        _snapshot_to_load = -1;
        return false;
    }

    Tick last_tick = _cmd_receiver.GetLoadedReplayLastTick();
    Tick new_tick = static_cast<Tick>(percent * last_tick + 0.5);

    const auto &snapshots = _options.snapshots;

    // Check the closest earlier snapshot and load it.
    auto it = lower_bound(snapshots.begin(), snapshots.end(), new_tick);
    if (it == snapshots.end()) it --;

    // Load the snapshot.
    _snapshot_to_load = *it;
    return true;
}

bool RTSGame::change_simulation_speed(float fraction) {
    _options.main_loop_quota /= fraction;
    return true;
}

CmdReturn RTSGame::dispatch_cmds(const UICmd& cmd) {
    switch(cmd.cmd) {
        case UI_SLIDEBAR:
            // UI controls, only works if there is a spectator.
            // cout << "Receive slider bar notification " << cmd.arg2 << endl;
            if (_spectator != nullptr && move_to_tick(cmd.arg2 / 100)) return CMD_QUEUE_RELOADED;
            break;
        case UI_FASTER_SIMULATION:
            if (_spectator != nullptr && change_simulation_speed(1.25)) return CMD_SUCCESS;
            break;
        case UI_SLOWER_SIMULATION:
            if (_spectator != nullptr && change_simulation_speed(0.8)) return CMD_SUCCESS;
            break;
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
        case TOGGLE_GAME_PAUSE:
            _paused = ! _paused;
            break;
        default:
            cout << "Cmd not handled! " << cmd.PrintInfo() << endl;
            break;
    }
    return CMD_FAILED;
}

void RTSGame::save_to_string(string *s) const {
    serializer::saver saver(true);
    _env.SaveSnapshot(saver);
    _cmd_receiver.SaveCmdReceiver(saver);
    *s = saver.get_str();
}

void RTSGame::load_from_string(const string &s) {
    serializer::loader loader(true);
    loader.set_str(s);
    _env.LoadSnapshot(loader);
    _cmd_receiver.LoadCmdReceiver(loader);
}

void RTSGame::load_snapshot(const string &filename) {
    serializer::loader loader(_options.save_with_binary_format);
    if (! loader.read_from_file(filename)) {
        throw std::range_error("Cannot read from " + filename);
    }
    _env.LoadSnapshot(loader);
    _cmd_receiver.LoadCmdReceiver(loader);
}

void RTSGame::save_snapshot(const string &filename) const {
    serializer::saver saver(_options.save_with_binary_format);
    _env.SaveSnapshot(saver);
    _cmd_receiver.SaveCmdReceiver(saver);
    if (! saver.write_to_file(filename)) {
        throw std::range_error("Cannot write to " + filename);
    }
}

void RTSGame::Reset() {
   _cmd_receiver.ResetTick();
   _cmd_receiver.ClearCmd();
   _env.Reset();
   // Send message to AIs.
   for (const auto &bot : _bots) {
       bot->Reset();
   }
}

PlayerId RTSGame::Step(int num_ticks, std::string *state) {
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

bool RTSGame::PrepareGame() {
  _cmd_receiver.SetVerbose(_options.cmd_verbose, 0);

  const unsigned int game_counter = _env.GetGameCounter();

  // Set the command dumper if there is any file specified.
  if (! _options.cmd_dumper_prefix.empty()) {
      const string filename = _options.cmd_dumper_prefix + "-" + std::to_string(game_counter) + ".cmd";
      if (_output_stream) *_output_stream << "Setup cmd_dumper. filename = " << filename << endl << flush;
      _cmd_receiver.SetCmdDumper(filename);
  }

  _cmd_receiver.SetUseCmdComment(! _options.save_replay_prefix.empty() || ! _options.cmd_dumper_prefix.empty() );

  /*
  if (! _options.map_filename.empty()) {
      _cmd_receiver.SendCmd(Cmd().SetLoadMap(_options.map_filename));
  }
  */

  // Load the replay (if there is any)
  const string& load_replay_filename =
      game_counter < _options.load_replay_filenames.size() ? _options.load_replay_filenames[game_counter] : string();

  // Load the replay.
  bool situation_loaded = false;
  if (! load_replay_filename.empty()) {
      if (_output_stream) *_output_stream << "Load from replay, name = " << load_replay_filename << endl << flush;
      if (_cmd_receiver.LoadReplay(load_replay_filename)) {
        situation_loaded = true;
      } else {
          if (_output_stream) *_output_stream << "Failed to open " << load_replay_filename << endl << flush;
          return false;
      }
  }
  if (! situation_loaded && ! _options.state_string.empty()) {
      if (_output_stream) *_output_stream << "Load from state string, size = " << _options.state_string.size() << endl << flush;
      load_from_string(_options.state_string);
      if (_output_stream) *_output_stream << "Finish load from state string" << endl << flush;
      situation_loaded = true;
  }
  if (! situation_loaded && ! _options.snapshot_load.empty()) {
      if (_output_stream) *_output_stream << "Loading snapshot = " << _options.snapshot_load << endl << flush;
      load_snapshot(_options.snapshot_load);
      situation_loaded = true;
  }

  if (! situation_loaded) {
      // Generate map.
      // _cmd_receiver.SendCmd(Cmd(0, INVALID).SetRandomSeed(1480918688));
      uint64_t seed = 0;
      if (_options.seed == 0) {
          auto now = system_clock::now();
          auto now_ms = time_point_cast<milliseconds>(now);
          auto value = now_ms.time_since_epoch();
          long duration = value.count();
          seed = (time(NULL) * 1000 + duration) % 100000000;
      } else {
          seed = _options.seed;
          if (game_counter > 0) {
              uint64_t seed_ext = seed;
              serializer::hash_combine(seed_ext, (uint64_t)(25147 * game_counter + 251581));
              seed = seed_ext;
          }
      }

      if (_output_stream) *_output_stream << "Generate from scratch, seed = " << seed << endl << flush;
      _cmd_receiver.SendCmdWithTick(CmdBPtr(new CmdRandomSeed(INVALID, seed)), 0);

      for (auto&& cmd_pair : _env.GetGameDef().GetInitCmds(_options)) {
          _cmd_receiver.SendCmdWithTick(std::move(cmd_pair.first), cmd_pair.second);
      }
  }
  return true;
}

PlayerId RTSGame::MainLoop(const std::atomic_bool *done) {
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
          for (const auto &bot : _bots) {
              *_output_stream << bot->PlotStructuredState(_env);
          }
          *_output_stream << _env.PrintDebugInfo() << flush;
          *_output_stream << "Acting ... " << flush << endl;
      }
      /*
      if (_output_stream) {
          uint64_t code = _env.CurrentHashCode();
          *_output_stream << "[" << t << "]: Current hash code: " << hex << code << dec << endl;
      }
      */

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
      if (winner_id != INVALID || _cmd_receiver.GetTick() >= _options.max_tick || ! _cmd_receiver.CheckGameSmooth(_output_stream)) {
          _env.SetTermination();
          if (winner_id != INVALID) {
              _bots[winner_id]->SendComment("Won");
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
