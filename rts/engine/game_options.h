#pragma once
#include <string>
#include <vector>
#include <sstream>
#include "common.h"

using namespace std;

struct RTSGameOptions {
    // A map file that specifies the map, the terrain
    string map_filename;

    string save_replay_prefix;
    string snapshot_prefix;
    string snapshot_load;
    string snapshot_load_prefix;

    // Snapshots to load from.
    vector<Tick> snapshots;

    // Initial seed. If seed = 0, then we use time(NULL)
    // When seed != 0, the game should be deterministic (If not then there is a bug somewhere).
    int seed = 0;

    // CmdDumperPrefix
    string cmd_dumper_prefix;

    // Ticks to be peeked. We only use this when bypass_bot_actions = true.
    set<Tick> peek_ticks;

    // Whether we show command during simulation.
    int cmd_verbose = 0;

    // Print Interval, in ticks.
    int tick_prompt_n_step = 2000;

    // Which file the intermediate prompt will be sent to.
    // if output_file == "cout", then it will send to stdout.
    // otherwise open a file and send the prompts to the file.
    string output_file;

    // If output_file is empty, then we check output_stream, if it is not nullptr,
    // we will use it for output.
    ostream *output_stream = nullptr;

    // Whether we save the snapshot using binary format (faster).
    bool save_with_binary_format = true;

    // time allowed to spend in main_loop, in milliseconds.
    int main_loop_quota = 0;

    // Max tick for the game to run.
    int max_tick = 30000;

    // Handicap_level used in Capture the Flag.
    int handicap_level = 0;

    bool team_play = false;

    string PrintInfo() const {
        std::stringstream ss;

        ss << "Map_filename: " << map_filename << endl;
        ss << "Save replay prefix: \"" << save_replay_prefix << "\"" << endl;
        ss << "Snapshot prefix: \"" << snapshot_prefix << "\"" << endl;
        ss << "Snapshot load: \"" << snapshot_load << "\"" << endl;
        ss << "Snapshot load prefix: \"" << snapshot_load_prefix << "\"" << endl;
        ss << "Snapshots[" << snapshots.size() << "]: ";
        for (const Tick &t : snapshots) ss << t << ", ";
        ss << endl;
        ss << "Seed: " << seed << endl;
        ss << "Cmd Dumper prefix: \"" << cmd_dumper_prefix << "\"" << endl;
        ss << "Pick ticks[" << peek_ticks.size() << "]: ";
        for (const Tick &t : peek_ticks) ss << t << ", ";
        ss << endl;
        ss << "Main Loop quota: " << main_loop_quota << endl;
        ss << "Cmd Verbose: " << (cmd_verbose ? "True" : "False") << endl;
        ss << "Output file: " << output_file << endl;
        ss << "Output stream: " << (output_stream ? "Not Null" : "Null") << endl;
        ss << "Max ticks: " << max_tick << endl;
        ss << "Tick prompt n step: " << tick_prompt_n_step << endl;
        ss << "Save with binary format: " << (save_with_binary_format ? "True" : "False") << endl;

        return ss.str();
    }
};

