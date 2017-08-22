/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

//File: main_loop.cc
//Author: Yuandong Tian <yuandong.tian@gmail.com>
//
#include "engine/common.h"
#include "engine/unit.h"
#include "engine/game.h"
#include "engine/cmd_util.h"
#include "engine/ai.h"
#include "ai.h"
#include "comm_ai.h"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <limits>
#include <vector>
#include <unordered_map>
#include <set>
#include <map>
#include <memory>
#include <chrono>
#include <thread>

using Parser = CmdLineUtils::CmdLineParser;

bool add_players(const string &args, int frame_skip, RTSGame *game) {
    vector<AI*> bots;
    //bool mcts = false;
    for (const auto& player : split(args, ',')) {
        cout << "Dealing with player = " << player << endl;
        if (player.find("tcp") == 0) {
            vector<string> params = split(player, '=');
            int tick_start = (params.size() == 1 ? 0 : std::stoi(params[1]));
            bots.push_back(new TCPAI("tcpai", tick_start, 8000, nullptr));
        }
        /*else if (player.find("mcts") == 0) {
            vector<string> params = split(player, '=');
            int mcts_thread = std::stoi(params[1]);
            int mcts_rollout_per_thread = std::stoi(params[2]);
            vector<string> prerun_cmds;
            if (params.size() >= 4) {
                prerun_cmds = split(params[3], '-');
            }
            bots.push_back(new MCTSAI(INVALID, frame_skip, nullptr, mcts_thread, mcts_rollout_per_thread, false, &prerun_cmds));
            mcts = true;
        }*/
        else if (player.find("spectator") == 0) {
            vector<string> params = split(player, '=');
            int tick_start = (params.size() == 1 ? 0 : std::stoi(params[1]));
            game->AddSpectator(new TCPAI("spectator", tick_start, 8000, game->GetCmdReceiver()));
        }
        else if (player == "dummy") bots.push_back(new AI("dummy", frame_skip, nullptr));
        /*
        else if (player == "flag_simple") {
            //if (mcts) bots[0]->SetFactory([&](int r) -> AI* { return new FlagSimpleAI(INVALID, r, nullptr, nullptr);});
            bots.push_back(new FlagSimpleAI(INVALID, frame_skip, nullptr));
        }*/
        //else if (player == "td_simple") bots.push_back(new TDSimpleAI(INVALID, frame_skip, nullptr));
        //else if (player == "td_built_in") bots.push_back(new TDBuiltInAI(INVALID, frame_skip, nullptr));
        else {
            AI *ai = AI::CreateAI(player, std::to_string(frame_skip));
            if (ai != nullptr) {
                bots.push_back(ai);
            } else {
                cout << "Unknown player! " << player << endl;
                return false;
            }
        }
    }
    for (auto bot : bots) {
        game->AddBot(bot);
    }
    cout << "Done with adding players" << endl << flush;
    return true;
}

RTSGameOptions GetOptions(const Parser &parser) {
    RTSGameOptions options;
    int vis_after = parser.GetItem<int>("vis_after", -1);

    options.cmd_dumper_prefix = parser.GetItem<string>("cmd_dumper_prefix", "");
    options.main_loop_quota = vis_after >= 0 ? 20 : 0;

    string replays = parser.GetItem<string>("load_replay", "");
    if (replays != "") {
        for (const auto &replay : split(replays, ',')) {
            options.load_replay_filenames.push_back(replay);
        }
    }

    options.save_replay_prefix = parser.GetItem<string>("save_replay", "replay");
    options.snapshot_load_prefix = parser.GetItem<string>("load_snapshot_prefix", "");
    options.snapshot_prefix = parser.GetItem<string>("save_snapshot_prefix", "");
    options.max_tick = parser.GetItem<int>("max_tick");
    options.output_file = parser.GetItem<string>("output_file", "");
    options.save_with_binary_format = parser.GetItem<bool>("binary_io");
    options.tick_prompt_n_step = parser.GetItem<int>("tick_prompt_n_step");
    options.seed = parser.GetItem<int>("seed");
    options.cmd_verbose = parser.GetItem<int>("cmd_verbose");
    options.handicap_level = parser.GetItem<int>("handicap_level", 0);

    string ticks = parser.GetItem<string>("peek_ticks", "");
    for (const auto &tick : split(ticks, ',')) {
        options.peek_ticks.insert((Tick)stoi(tick));
    }

    if (parser.HasItem("load_snapshot_length")) {
        int snapshot_len = parser.GetItem<int>("load_snapshot_length");
        for (Tick i = 0; i < snapshot_len; ++i) {
            options.snapshots.push_back(i);
        }
    }

    return options;
}

RTSGameOptions ai_vs_human(const Parser &parser, string *players) {
    RTSGameOptions options = GetOptions(parser);
    *players = "tcp,simple";
    options.main_loop_quota = 40;

    return options;
}

RTSGameOptions ai_vs_ai(const Parser &parser, string *players) {
    RTSGameOptions options = GetOptions(parser);
    *players = "simple,simple";
    int vis_after = parser.GetItem<int>("vis_after");
    if (vis_after >= 0) *players += ",spectator=" + std::to_string(parser.GetItem<int>("vis_after"));

    return options;
}

RTSGameOptions ai_vs_ai2(const Parser &parser, string *players) {
    RTSGameOptions options = GetOptions(parser);
    *players = "simple,hit_and_run";
    int vis_after = parser.GetItem<int>("vis_after");
    if (vis_after >= 0) *players += ",spectator=" + std::to_string(parser.GetItem<int>("vis_after"));

    return options;
}
/*
RTSGameOptions ai_vs_mcts(const Parser &parser, string *players) {
    RTSGameOptions options = GetOptions(parser);
    int mcts_threads = parser.GetItem<int>("mcts_threads");
    int mcts_rollout_per_thread = parser.GetItem<int>("mcts_rollout_per_thread");
    string str_prerun_cmds = parser.GetItem<string>("mcts_prerun_cmds", "");

    *players = "mcts=" + to_string(mcts_threads) + "=" + to_string(mcts_rollout_per_thread);
    if (! str_prerun_cmds.empty()) *players + "=" + str_prerun_cmds;
    *players += ",simple";

    int vis_after = parser.GetItem<int>("vis_after");
    if (vis_after >= 0) *players += ",spectator=" + std::to_string(parser.GetItem<int>("vis_after"));

    return options;
}
*/
RTSGameOptions flag_ai_vs_ai(const Parser &parser, string *players) {
    RTSGameOptions options = GetOptions(parser);
    *players = "flag_simple,flag_simple,dummy";
    int vis_after = parser.GetItem<int>("vis_after");
    if (vis_after >= 0) *players += ",spectator=" + std::to_string(parser.GetItem<int>("vis_after"));
    return options;
}

/*
RTSGameOptions flag_ai_vs_mcts(const Parser &parser, string *players) {
    RTSGameOptions options = GetOptions(parser);
    int mcts_threads = parser.GetItem<int>("mcts_threads");
    int mcts_rollout_per_thread = parser.GetItem<int>("mcts_rollout_per_thread");
    string str_prerun_cmds = parser.GetItem<string>("mcts_prerun_cmds", "");

    *players = "mcts=" + to_string(mcts_threads) + "=" + to_string(mcts_rollout_per_thread);
    if (! str_prerun_cmds.empty()) *players + "=" + str_prerun_cmds;
    *players += ",flag_simple";
    int vis_after = parser.GetItem<int>("vis_after");
    if (vis_after >= 0) *players += ",spectator=" + std::to_string(parser.GetItem<int>("vis_after"));
    return options;
}
*/
RTSGameOptions td_simple(const Parser &parser, string *players) {
    RTSGameOptions options = GetOptions(parser);
    *players = "td_simple,td_built_in";
    int vis_after = parser.GetItem<int>("vis_after");
    if (vis_after >= 0) *players += ",spectator=" + std::to_string(parser.GetItem<int>("vis_after"));
    return options;
}

RTSGameOptions replay_cmd(const Parser &parser, string *players) {
    RTSGameOptions options = GetOptions(parser);
    *players = "dummy,dummy";
    options.main_loop_quota = 0;
    options.bypass_bot_actions = true;
    return options;
}

RTSGameOptions replay(const Parser &parser, string *players) {
    RTSGameOptions options = GetOptions(parser);
    *players = "dummy,dummy,spectator=0";
    options.bypass_bot_actions = true;
    return options;
}
/*
void replay_mcts(const Parser &parser) {
    string filename = parser.GetItem<string>("load_binary_string");
    //int mcts_threads = parser.GetItem<int>("mcts_threads");
    //int mcts_rollout_per_thread = parser.GetItem<int>("mcts_rollout_per_thread");
    //bool mcts_verbose = parser.GetItem<bool>("mcts_verbose", false);
    int frame_skip = parser.GetItem<int>("frame_skip", 1);
    string str_prerun_cmds = parser.GetItem<string>("mcts_prerun_cmds", "");
    vector<string> prerun_cmds = split(str_prerun_cmds, '-');

    cout << "Load filename = " << filename << endl;
    ifstream iFile(filename, ios::binary);
    if (! iFile.is_open()) {
        cout << "Error opening " << filename << endl;
        return;
    }
    std::stringstream ss;
    ss << iFile.rdbuf();
    iFile.close();
    cout << "string length = " << ss.str().size() << endl;

    // Load the game.
    RTSGameOptions options = GetOptions(parser);
    options.state_string = ss.str();

    cout << "Options: " << endl;
    cout << options.PrintInfo() << endl;

    cout << "Loading game " << endl;
    RTSGame game(options);
    game.AddBot(new MCTSAI(INVALID, frame_skip, nullptr, mcts_threads, mcts_rollout_per_thread, mcts_verbose, &prerun_cmds));
    game.AddBot(new SimpleAI(INVALID, frame_skip, nullptr, nullptr));

    cout << "Starting main loop " << endl;
    PlayerId winner = game.MainLoop();

    cout << "Winner " << winner << endl;
}

void replay_rollout(const Parser &parser) {
    vector<int> selected_moves;
    string filename = parser.GetItem<string>("load_binary_string");
    int frame_skip = parser.GetItem<int>("frame_skip", 1);
    cout << "Load filename = " << filename << endl;

    serializer::loader loader(false);
    if (! loader.read_from_file(filename + "_move")) {
        cout << "Error opening " << filename << + "_move" << endl;
        return;
    }
    loader >> selected_moves;
    cout << "Selected moves: " << selected_moves.size() << endl;

    cout << "Loading state_string " << filename << endl;
    ifstream iFile(filename, ios::binary);
    if (! iFile.is_open()) {
        cout << "Error opening " << filename << endl;
        return;
    }
    std::stringstream ss;
    ss << iFile.rdbuf();
    iFile.close();
    cout << "string length = " << ss.str().size() << endl;

    // Load the game.
    RTSGameOptions options = GetOptions(parser);
    options.state_string = ss.str();

    cout << "Options: " << endl;
    cout << options.PrintInfo() << endl;

    cout << "Loading game " << endl;
    RTSGame game(options);
    game.AddBot(new MCTS_ROLLOUT_AI(0, frame_skip, nullptr, selected_moves));
    game.AddBot(new SimpleAI(1, frame_skip, nullptr, nullptr));

    cout << "Starting main loop " << endl;
    PlayerId winner = game.MainLoop();

    cout << "Winner " << winner << endl;
}
*/
void test() {
    RTSMap m;
    vector<Player> players;
    for (int i = 0; i < 2; ++i) {
        players.emplace_back(m, i);
    }

    serializer::saver saver(false);
    saver << players;
    if (!  saver.write_to_file("tmp.txt")) {
        cout << "Write file error!" << endl;
    }

    serializer::loader loader(false);
    if (! loader.read_from_file("tmp.txt")) {
        cout << "Read file error!" << endl;
    }
    loader >> players;

    for (const auto &p : players) {
        cout << p.PrintInfo() << endl;
    }
}

int main(int argc, char *argv[]) {
    const map<string, function<RTSGameOptions (const Parser &, string *)> > func_mapping = {
        { "selfplay", ai_vs_ai },
        { "selfplay2", ai_vs_ai2 },
        //{ "mcts", ai_vs_mcts },

        { "replay", replay },
        { "replay_cmd", replay_cmd },
        { "humanplay", ai_vs_human },
        { "multiple_selfplay", nullptr},
        //{ "replay_rollout", nullptr},
        //{ "replay_mcts", nullptr},

        // capture the flag
        { "flag_selfplay", flag_ai_vs_ai },
        //{ "flag_mcts", flag_ai_vs_mcts },

        // tower defense
        { "td_simple", td_simple },
    };

    CmdLineUtils::CmdLineParser parser("playstyle --save_replay --load_replay --vis_after[-1] --save_snapshot_prefix --load_snapshot_prefix --seed[0] \
--load_snapshot_length --max_tick[30000] --binary_io[1] --games[16] --frame_skip[1] --tick_prompt_n_step[2000] --cmd_verbose[0] --peek_ticks --cmd_dumper_prefix \
--output_file[cout] --mcts_threads[16] --mcts_rollout_per_thread[100] --threads[64] --load_binary_string --mcts_verbose --mcts_prerun_cmds --handicap_level[0]");

    if (! parser.Parse(argc, argv)) {
        cout << parser.PrintHelper() << endl;
        return 0;
    }

    cout << "Cmd Options: " << endl;
    cout << parser.PrintParsed() << endl;
    auto playstyle = parser.GetItem<string>("playstyle");
    auto it = func_mapping.find(playstyle);
    if (it == func_mapping.end()) {
        cout << "Unknown command " << playstyle << "! Available commands are: " << endl;
        for (auto it = func_mapping.begin(); it != func_mapping.end(); ++it) {
            cout << it->first << endl;
        }
        return 0;
    }

    RTSGameOptions options;
    string players;
    try {
        if (it->second != nullptr) options = it->second(parser, &players);
    } catch(const std::exception& e) {
        cout << e.what() << endl;
        return 1;
    }

    int frame_skip = parser.GetItem<int>("frame_skip", 1);

    auto time_start = chrono::system_clock::now();
    // if (playstyle == "replay_rollout") {
    //    replay_rollout(parser);
    // } else if (playstyle == "replay_mcts") {
    //    replay_mcts(parser);
    // } else

    if (playstyle == "multiple_selfplay") {
        int threads = parser.GetItem<int>("threads");
        int games = parser.GetItem<int>("games");
        int seed0 = parser.GetItem<int>("seed");
        ctpl::thread_pool p(threads + 1);
        const int print_per_n = (games == 0 ? 5000 : games * threads / 10);
        GlobalStats gstats(print_per_n);

        for (int i = 0; i < threads; i++) {
            p.push([=, &gstats](int /*thread_id*/){
                RTSGameOptions options;
                options.main_loop_quota = 0;
                options.output_file = "";
                options.tick_prompt_n_step = -1;
                if (seed0 == 0) options.seed = 0;
                else options.seed = seed0 + i * 241;

                RTSGame game(options);
                //game.AddBot(new SimpleAI(INVALID, frame_skip, nullptr));
                //game.AddBot(new SimpleAI(INVALID, frame_skip, nullptr));
                game.AddBot(AI::CreateAI("simple", std::to_string(frame_skip)));
                game.AddBot(AI::CreateAI("simple", std::to_string(frame_skip)));
                game.GetCmdReceiver()->GetGameStats().SetGlobalStats(&gstats);
                bool infinite = (games == 0);
                for (int j = 0; j < games || infinite; ++j) {
                    game.MainLoop();
                    game.Reset();
                }
            });
            this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        p.stop(true);
        std::cout << gstats.PrintInfo() << std::endl;
    } else {
        RTSGame game(options);
        cout << "Players: " << players << endl;
        add_players(players, frame_skip, &game);
        cout << "Finish adding players" << endl;

        chrono::duration<double> duration = chrono::system_clock::now() - time_start;
        cout << "Total time spent = " << duration.count() << "s" << endl;
        if (options.load_replay_filenames.empty()) {
             game.MainLoop();
        } else {
            // Load replay etc.
            for (size_t i = 0; i < options.load_replay_filenames.size(); ++i) {
                game.MainLoop();
                game.Reset();
            }
        }
    }

    return 0;
}
