/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once

#include "elf/tree_search.h"
#include "board.h"

namespace go_mcts {

using namespace std;

// Tree search specialization for Go.
class State {
public:
    State() : value_(0.5) { }
    State(const Board &board) : value_(0.5) { CopyBoard(&board_, &board); }

    const vector<pair<Coord, float>> &pi() const { return pi_; }

    template <typename AIComm>
    bool GetPi(AIComm *ai_comm) {
        // ai_comm is from the current thread.
        auto &gs = ai_comm->Prepare();
        BoardFeature bf(board_);
        bf.Extract(&gs.s);

        if (!ai_comm->SendDataWaitReply()) return false;

        const vector<float> &pi = ai_comm->info().data.newest().pi;

        pi_.clear();
        for (size_t i = 0; i < pi.size(); ++i) {
            Coord m = bf.Action2Coord(i);
            pi_.push_back(make_pair(m, pi[i])); 
        }
        return true;
    }

    bool forward(const Coord &c, State *next) const {
        CopyBoard(&next->board_, &board_);
        return next->forward(c);
    }

    bool forward(const Coord &c) {
        GroupId4 ids;
        if (TryPlay2(&board_, c, &ids)) {
          Play(&board_, &ids);
          return true;
        } else {
          return false;
        }
    }

    // Evaluate using random playout.
    // Right now just set it to 0.5
    float evaluate() const { return value_; }
    const Board &board() const { return board_; }

private:
    Board board_;
    vector<pair<Coord, float>> pi_;
    float value_;
};

using GoMCTS = mcts::TreeSearchT<State, Coord>;
using TSOptions = mcts::TSOptions;
using TSThreadOptions = mcts::TSThreadOptionsT<State, Coord>;

template <typename AIComm>
TSThreadOptions GetTSThreadOptions(AIComm *ai_comm) {
    TSThreadOptions options;
    options.stats_func = [&](State *s) { return s->GetPi(ai_comm); };
    options.forward_func = [](const State &s, const Coord &c, State *s_next) { return s.forward(c, s_next); };
    options.evaluate = [](const State &s) { return s.evaluate(); };
    return options;
}

}  // namespace go_mcts
