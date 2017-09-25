#include "online_mcts_player.h"
#include "mcts.h"

OnlineMCTSPlayer::OnlineMCTSPlayer(int num_threads, AIComm *parent_ai_comm) 
    : Loader(parent_ai_comm) {
    vector<go_mcts::TSThreadOptions> thread_options;
    for (int i = 0; i < num_threads; ++i) {
        ai_comms_.emplace_back(parent_ai_comm->Spawn(i));
        thread_options.push_back(go_mcts::GetTSThreadOptions(ai_comms_[i].get()));
    }
    mcts::TSOptions ts_options;
    ts_.reset(new go_mcts::GoMCTS(ts_options, thread_options));
}

////////////////////////////////////// Online MCTS Player /////////////////////////////////////////
void OnlineMCTSPlayer::Act(const atomic_bool &done) {
    (void)done;
    pair<Coord, float> next_move = ts_->Run(_state.board());
    cout << "Get next move: " << coord2str(next_move.first) << " #visited: " << next_move.second << endl; 
    _state.ApplyMove(next_move.first);
}

