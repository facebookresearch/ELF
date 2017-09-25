#include "game_state.h"

using namespace std;
using namespace std::chrono;

RTSState::RTSState() {
    _env.InitGameDef();
    // TODO Need to add players accordingly. 
    _env.ClearAllPlayers();
}

void RTSState::OnAddPlayer(int player_id) {
    (void)player_id;
    _env.AddPlayer(PV_KNOW_ALL);
}

void RTSState::OnRemovePlayer(int player_id) {
    (void)player_id;
    _env.RemovePlayer();
}

bool RTSState::Prepare(const RTSGameOptions &options, ostream *output) {
    _cmd_receiver.SetVerbose(options.cmd_verbose, 0);

    const unsigned int game_counter = _env.GetGameCounter();

    // Set the command dumper if there is any file specified.
    if (! options.cmd_dumper_prefix.empty()) {
        const string filename = options.cmd_dumper_prefix + "-" + std::to_string(game_counter) + ".cmd";
        if (output) *output << "Setup cmd_dumper. filename = " << filename << endl << flush;
        _cmd_receiver.SetCmdDumper(filename);
    }

    _cmd_receiver.SetUseCmdComment(! options.save_replay_prefix.empty() || ! options.cmd_dumper_prefix.empty() );

    /*
       if (! options.map_filename.empty()) {
       _cmd_receiver.SendCmd(Cmd().SetLoadMap(options.map_filename));
       }
       */

    // Load the replay (if there is any)
    const string& load_replay_filename =
        game_counter < options.load_replay_filenames.size() ? options.load_replay_filenames[game_counter] : string();

    // Load the replay.
    bool situation_loaded = false;
    if (! load_replay_filename.empty()) {
        if (output) *output << "Load from replay, name = " << load_replay_filename << endl << flush;
        if (_cmd_receiver.LoadReplay(load_replay_filename)) {
            situation_loaded = true;
        } else {
            if (output) *output << "Failed to open " << load_replay_filename << endl << flush;
            return false;
        }
    }
    if (! situation_loaded && ! options.state_string.empty()) {
        if (output) *output << "Load from state string, size = " << options.state_string.size() << endl << flush;
        Load(options.state_string);
        if (output) *output << "Finish load from state string" << endl << flush;
        situation_loaded = true;
    }
    if (! situation_loaded && ! options.snapshot_load.empty()) {
        if (output) *output << "Loading snapshot = " << options.snapshot_load << endl << flush;
        LoadSnapshot(options.snapshot_load, options.save_with_binary_format);
        situation_loaded = true;
    }

    if (! situation_loaded) {
        // Generate map.
        // _cmd_receiver.SendCmd(Cmd(0, INVALID).SetRandomSeed(1480918688));
        uint64_t seed = 0;
        if (options.seed == 0) {
            auto now = system_clock::now();
            auto now_ms = time_point_cast<milliseconds>(now);
            auto value = now_ms.time_since_epoch();
            long duration = value.count();
            seed = (time(NULL) * 1000 + duration) % 100000000;
        } else {
            seed = options.seed;
            if (game_counter > 0) {
                uint64_t seed_ext = seed;
                serializer::hash_combine(seed_ext, (uint64_t)(25147 * game_counter + 251581));
                seed = seed_ext;
            }
        }

        if (options.map_filename.empty()) {
            if (output) *output << "Generate from scratch, seed = " << seed << endl << flush;
            _cmd_receiver.SendCmdWithTick(CmdBPtr(new CmdRandomSeed(INVALID, seed)), 0);
        } else {
            // _cmd_receiver.SendCmdWithTick(CmdBPtr(new CmdLoadMap(INVALID, options.map_filename)));
        }

        for (auto&& cmd_pair : _env.GetGameDef().GetInitCmds(options)) {
            _cmd_receiver.SendCmdWithTick(std::move(cmd_pair.first), cmd_pair.second);
        }
    }
    _max_tick = options.max_tick;
    return true;
}

// 0.0 - 1.0
int RTSState::MoveToTick(const std::vector<Tick> &snapshots, float percent) const {
    // Move to a specific tick.
    int num_replay_entry = _cmd_receiver.GetLoadedReplaySize();
    cout << "#replay = " << num_replay_entry << endl;
    cout << "#snapshot = " << snapshots.size() << endl;

    if (num_replay_entry == 0)  return -1;

    Tick last_tick = _cmd_receiver.GetLoadedReplayLastTick();
    Tick new_tick = static_cast<Tick>(percent * last_tick + 0.5);

    // Check the closest earlier snapshot and load it.
    auto it = lower_bound(snapshots.begin(), snapshots.end(), new_tick);
    if (it == snapshots.end()) it --;

    // Load the snapshot.
    return *it;
}

bool RTSState::Reset() {
   _cmd_receiver.ResetTick();
   _cmd_receiver.ClearCmd();
   _env.Reset();
   return true;
}

void RTSState::Forward(RTSAction &action) {
    action.Send(_env, _cmd_receiver);
}

elf::GameResult RTSState::PostAct() {
    _env.Forward(&_cmd_receiver);
    // if (_tick_prompt) *_output_stream << "Start executing cmds... " << endl << flush;
    _cmd_receiver.ExecuteDurativeCmds(_env, _verbose);
    _cmd_receiver.ExecuteImmediateCmds(&_env, _verbose);
    _cmd_receiver.ExecuteUICmds(_ui_cb);
    // cout << "Compute Fow" << endl;
    _env.ComputeFOW();

    // Check winner. 
    PlayerId winner_id = _env.GetGameDef().CheckWinner(_env, _cmd_receiver.GetTick() >= _max_tick);
    _env.SetWinnerId(winner_id);


    Tick t = _cmd_receiver.GetTick();
    bool run_normal = _cmd_receiver.GetGameStats().CheckGameSmooth(t);

    // Check winning condition
    if (winner_id != INVALID || t >= _max_tick || ! run_normal) {
        _env.SetTermination();
        _cmd_receiver.GetGameStats().SetWinner(winner_id);
        return run_normal ? elf::GAME_END : elf::GAME_ERROR; 
    } 
    
    return elf::GAME_NORMAL;
}

void RTSState::Finalize() {
    // cout << "[" << prefix << "] About to save to rep" << endl;
    if (! _save_replay_prefix.empty()) {
        const int game_counter = _env.GetGameCounter();
        std::string prefix = _save_replay_prefix + std::to_string(game_counter);
        _cmd_receiver.SaveReplay(prefix + ".rep");
    }
}

