# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

#!/usr/bin/env python
# -*- coding: utf-8 -*-
# File: benchmark-atari.py
# Author: Yuxin Wu <ppwwyyxxc@gmail.com>

import random
import time
from time import sleep
import numpy as np
import sys

from six.moves import range
import atari_game as atari
from game_utils import initialize_game
from collections import Counter

Niter = 30000
frame_skip = 4

def benchmark(ngame):
    batchsize = 16
    co, opt = initialize_game(
        batchsize, num_games=ngame,
        frame_skip=frame_skip, T=4)
    GC = atari.GameContext(co, opt)
    start = time.time()
    sys.stderr.write("Start benchmark ngame={}...\n".format(ngame))

    stats = Counter()
    for i in range(ngame): stats[i] = 0

    for k in range(Niter):
        infos = GC.Wait(0)
        #for i in range(len(infos)):
        #    stats[infos[i].meta().id] += 1
        GC.Steps(infos)
    print(stats)
    return Niter / (time.time() - start) * frame_skip * batchsize

if __name__ == '__main__':
    for ngame in [64, 128, 256, 512, 1024]:
        fps = benchmark(ngame)
        print("NGame={}, FPS={}".format(ngame, fps))
