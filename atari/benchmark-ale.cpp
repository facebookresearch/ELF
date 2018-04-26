/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

//File: benchmark-ale.cpp
//Author: Yuxin Wu <ppwwyyxxc@gmail.com>
//g++ -O3 benchmark-ale.cc -std=c++11 `pkg-config --cflags --libs ale`

#include <iostream>
#include <cstring>
#include <cstdio>
#include <thread>
#include <limits>
#include <vector>
#include <unordered_map>
#include <set>
#include <iterator>
#include <unordered_set>
#include <queue>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <ale/ale_interface.hpp>
#include <atomic>

#include "timer.hh"
#include "blockingconcurrentqueue.h"

using namespace std;

std::mutex g_mutex;
condition_variable g_cv;

std::vector<int*> counters;

constexpr const int Nth = 1;

std::vector<int> avail(Nth, 0);
std::atomic_int cntavail{0};

double benchmark(int job) {
    (void)job;
    std::unique_ptr<ALEInterface> _ale;

    int i = 0;
    {
      std::lock_guard<mutex> lg(g_mutex);
      _ale.reset(new ALEInterface);
      _ale->setInt("random_seed", 53);
      _ale->setBool("showinfo", false);
      _ale->setInt("frame_skip", 4);
      _ale->setBool("color_averaging", false);
      _ale->setFloat("repeat_action_probability", 0.0);
      _ale->loadROM("pong.bin");

      counters.push_back(&i);
    }
    {
      std::unique_lock<mutex> lk(g_mutex);
      g_cv.wait(lk);
    }

    auto actions = _ale->getMinimalActionSet();
    int nr_action = actions.size();

    int N = 10000;
    Timer tm;
    for (i = 0; i < N; ++i) {
      // auto tick = _ale->getEpisodeFrameNumber();
      bool terminated = _ale->game_over();
      // int l = _ale->lives();
      if (terminated)
        _ale->reset_game();
      vector<unsigned char> buf;
      _ale->getScreenRGB(buf);
      int act = rand() % nr_action;
      _ale->act(actions[act]);
    }
    cout << N / tm.duration() << endl;
    return N / tm.duration();
}


void timer() {
  Timer tm;

  while (true) {
    this_thread::sleep_for(chrono::seconds(3));
    int sum = 0;
    for (auto& ptr : counters) {
      sum += *ptr;
    }
    cout << "Total it/second" << sum / tm.duration() << endl;
  }
}

int main() {
  ale::Logger::setMode(ale::Logger::mode::Error);
  std::vector<thread> ths;
  for (int i = 0; i < Nth; ++i) {
    ths.emplace_back(thread{benchmark, i});
  }
  this_thread::sleep_for(chrono::seconds(5));

  while (counters.size() != Nth) {
    cout << "Init"<< counters.size() << endl;
    this_thread::sleep_for(chrono::seconds(1));
  }
  g_cv.notify_all();
  cout << "starting..." << endl;
  std::thread y([](){timer();});
  y.detach();

  for (auto& t: ths)
    t.join();

}
