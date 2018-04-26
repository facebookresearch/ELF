/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "tree_search.h"
#include <iostream>
#include <vector>
#include <utility>

using namespace std;

struct State {
public:
    const int kMaxS = 10; 

    State() { s_ = 0; }

    void set_thread(int) { }
    bool forward(int a) {
        if (s_ == kMaxS) return false;

        if (a == 0) s_ --;
        else if (a == 1) s_ ++;
        if (s_ < 0) s_ = 0;
        return true;
    }

    float reward() const { return s_ == kMaxS ? 1.0 : 0.0; }
    void evaluate() { 
        pi_.clear();
        pi_.push_back(make_pair(0, 0.5));
        pi_.push_back(make_pair(1, 0.5));
        V_ = 0.5;
    }
    
    const vector<pair<int, float>> &pi() const { return pi_; }
    float value() const { return V_; }

private:
    vector<pair<int, float>> pi_;
    float V_;
    int s_;
};

int main() {
    mcts::TSOptions options;
    options.num_threads = 16;
    options.num_rollout_per_thread = 100;
    mcts::TreeSearchT<State, int> tree_search(options);

    State s;
    
    for (int i = 0; i < 10; ++i) {
        pair<int, float> res = tree_search.Run(s);
        cout << "Action: " << res.first << " Score:" << res.second << endl << flush;
        tree_search.TreeAdvance(res.first);
        s.forward(res.first);
    }

    tree_search.Stop();
    return 0;
}
