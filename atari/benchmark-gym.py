# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

#!/usr/bin/env python
# -*- coding: utf-8 -*-
# File: benchmark-gym.py
# Author: Yuxin Wu <ppwwyyxxc@gmail.com>

import os
import sys
import numpy as np
import time
import tqdm
import threading
import queue
import gym
import multiprocessing as mp

from tensorpack.utils.concurrency import ensure_proc_terminate

ALELOCK = threading.Lock()

def bench_proc():
    Q = mp.Queue()

    def work():
        player = gym.make('PongDeterministic-v3')
        naction = player.action_space.n
        np.random.seed(os.getpid())
        player.reset()

        while True:
            act = np.random.choice(naction)
            ob, r, isOver, info = player.step(act)
            Q.put([ob, r])
            if isOver:
                player.reset()
    nr_proc = 8
    procs = [mp.Process(target=work) for _ in range(nr_proc)]
    ensure_proc_terminate(procs)
    for p in procs:
        p.start()

    for _ in tqdm.trange(100000):
        Q.get()

def bench_thread(ngame):
    Niter = 80000 * (ngame // 64)
    Q = queue.Queue(maxsize=Niter + 10000)
    nr_proc = ngame
    barrier = threading.Barrier(nr_proc+ 1)

    evt = threading.Event()

    def work():
        with ALELOCK:
            player = gym.make('PongDeterministic-v3')
        naction = player.action_space.n
        np.random.seed(os.getpid())
        player.reset()
        barrier.wait()

        while not evt.isSet():
            act = np.random.choice(naction)
            ob, r, isOver, info = player.step(act)
            Q.put([ob, r])
            if isOver:
                player.reset()
    procs = [threading.Thread(target=work, daemon=True) for _ in range(nr_proc)]
    for p in procs:
        p.start()
    barrier.wait()
    print("Start benchmarking ngame={}...".format(ngame), file=sys.stderr)

    start = time.time()
    for t in range(Niter):
        Q.get()
    fps = Niter / (time.time() - start) * 4
    evt.set()
    print("NGame={}, FPS={}".format(ngame, fps))
    return fps

if __name__ == '__main__':
    ngame = int(sys.argv[1])
    #bench_proc()
    fps = bench_thread(ngame)
