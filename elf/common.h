/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

struct SeqInfo {
    int seq;
    int game_counter;
    char last_terminal;

    SeqInfo() : seq(0), game_counter(0), last_terminal(0) { }
    void Inc() { seq ++; last_terminal = 0; }
    void NewEpisode() { seq = 0; game_counter ++; last_terminal = 1; }
};


