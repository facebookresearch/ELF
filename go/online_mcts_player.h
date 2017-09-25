#pragma once

#include "go_game_specific.h"
#include "go_loader.h"
#include "mcts.h"

using namespace std;

class OnlineMCTSPlayer: public Loader {
public:
    OnlineMCTSPlayer(int num_threads, AIComm *parent_ai_comm);
    void Act(const atomic_bool &done) override; 

private:
    vector<unique_ptr<AIComm>> ai_comms_;
    unique_ptr<go_mcts::GoMCTS> ts_;
};

