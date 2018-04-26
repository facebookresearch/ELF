/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

//File: benchmark-atari.cpp
//g++ -O3 benchmark-atari.cc atari_game.cc -std=c++11 `pkg-config --cflags --libs ale` -I../vendor `python-config --cflags --ldflags` -DUSE_PYBIND11 && ./a.out

#include <iostream>
#include <cstring>
#include <cstdio>
#include <limits>
#include <vector>
#include <unordered_map>
#include <set>
#include <iterator>
#include <unordered_set>
#include <queue>
using namespace std;
#include "timer.hh"
#include "atari_game.h"
#include "game_context.h"

int main() {
  ContextOptions co;
  co.num_games = 256;

  GameOptions go;
  go.rom_file = "pong.bin";
  go.frame_skip = 4;
  go.seed = 42;

  GameContext GC{co, go};

  int batchsize = 16;
  GroupStat stat;
  GC.AddCollectors(batchsize, 1, 0, stat);

  cout << "Start timing ..." << endl;
  int N = 30000;
  Timer tm;
  GC.Start();
  for (int i = 0; i < N; ++i) {
    auto v = GC.Wait(0);
    GC.Steps(v);
    if (i == 0) {
      tm.restart();
    }
  }
  GC.Stop();
  cout << "tm duration" << tm.duration() << endl;
  cout << "seconds per iter:" << tm.duration() / N << endl;
  cout << N / tm.duration() * batchsize * go.frame_skip << endl;
}
