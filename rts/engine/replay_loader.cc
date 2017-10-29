#include "replay_loader.h"
#include "game_state.h"

#include "cmd.gen.h"
#include "serializer.h"
#include "elf/tar_loader.h"
//
bool ReplayLoader::Load(const string& replay_filename) {
    // Load the replay_file (which is a action sequence)
    // Each action looks like the following:
    //    Tick, CmdType, UnitId, UnitType, Loc
    //
    if (replay_filename.empty()) return false;

    _loaded_replay.clear();
    _next_replay_idx = 0;

    // cout << "Loading replay = " << replay_filename << endl;
    serializer::loader loader(false);

    auto pos = replay_filename.find(".tar?");
    if (pos != string::npos) {
        // Tar file.
        const string tar_file = replay_filename.substr(0, pos) + ".tar";
        const string sub_file = replay_filename.substr(pos + 5, string::npos);
        elf::tar::TarLoader tar_loader(tar_file);
        loader.set_str(tar_loader.Load(sub_file));
    } else {
        if (! loader.read_from_file(replay_filename)) {
            cout << "Loaded replay " << replay_filename << " failed!" << endl;
            return false;
        }
    }
    _loaded_replay.clear();
    loader >> _loaded_replay;
    cout << "Loaded replay, size = " << _loaded_replay.size() << endl;

    return true;
}

void ReplayLoader::SendReplay(Tick tick, Action *actions) {
    actions->cmds.clear();
    actions->restart = (tick == 0 && ! _loaded_replay.empty());

    while (_next_replay_idx <  _loaded_replay.size()) {
        const auto& cmd = _loaded_replay[_next_replay_idx];
        if (cmd->tick() > tick) break;
        /*
        if (cmd->type() != COMMENT) {
            cout << "Send Current Replay: " << cmd->PrintInfo() << endl;
        }
        */
        actions->cmds.emplace_back(cmd->clone());
        _next_replay_idx ++;
    }
    // cout << "Replay sent, #record = " << _next_replay_idx << endl;
}

void ReplayLoader::Relocate(Tick tick) {
    // Adjust the _next_replay_idx pointer so that it points to the position just before the tick. 
    _next_replay_idx = 0;
    while (_next_replay_idx < _loaded_replay.size()) {
        if (_loaded_replay[_next_replay_idx]->tick() >= tick) break;
        _next_replay_idx ++;
    }
}

bool Replayer::Act(const RTSState &s, Action *a, const atomic_bool *) {
  ReplayLoader::SendReplay(s.GetTick(), a);
  return true;
}


