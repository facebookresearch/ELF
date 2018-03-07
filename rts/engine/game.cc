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
            // Already handled by Spectator.
            break;
        case UI_CYCLEPLAYER:
            // Already dealt with by Specatator.
            break;
        case UI_FASTER_SIMULATION:
            if (change_simulation_speed(1.25)) return CMD_SUCCESS;
            break;
        case UI_SLOWER_SIMULATION:
            if (change_simulation_speed(0.8)) return CMD_SUCCESS;
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

    if (_paused) return elf::GAME_NORMAL;

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

