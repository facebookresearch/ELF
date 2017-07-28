# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

#!/usr/bin/env python
# -*- coding: utf-8 -*-
# File: game.py

from datetime import datetime
from collections import Counter
import argparse
from time import sleep
import os
import atari_game as atari
import torch

import sys
sys.path.append('../')

from elf import GCWrapper, ContextArgs
from rlpytorch import ArgsProvider

class Loader:
    def __init__(self):
        self.context_args = ContextArgs()

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("frame_skip", 4),
                ("hist_len", 4),
                ("rom_file", "pong.bin"),
                ("actor_only", dict(action="store_true")),
                ("reward_clip", 1),
                ("rom_dir", os.path.dirname(__file__))
            ],
            more_args = ["batchsize", "T"],
            child_providers = [ self.context_args.args ]
        )

    def initialize(self):
        args = self.args
        co = atari.ContextOptions()
        self.context_args.initialize(co)

        opt = atari.Options()
        opt.frame_skip = args.frame_skip
        opt.rom_file = os.path.join(args.rom_dir, args.rom_file)
        opt.seed = 42
        opt.hist_len = args.hist_len
        opt.reward_clip = args.reward_clip

        GC = atari.GameContext(co, opt)
        print("Version: ", GC.Version())

        num_action = GC.get_num_actions()
        print("Num Actions: ", num_action)

        desc = {}
        # For actor model, No reward needed, we only want to get input and return distribution of actions.
        # sampled action and and value will be filled from the reply.

        desc["actor"] = dict(
            input=dict(batchsize=args.batchsize, T=1, keys=["s", "last_r", "last_terminal"]),
            reply=dict(batchsize=args.batchsize, T=1, keys=["rv", "pi", "V", "a"])
        )

        if not args.actor_only:
            # For training: group 1
            # We want input, action (filled by actor models), value (filled by actor
            # models) and reward.
            desc["train"] = dict(
                input=[dict(batchsize=args.batchsize, T=args.T), ["rv", "id", "pi", "s", "a", "r", "V", "seq", "terminal"] ],
                reply=None
            )

        # Initialize shared memory (between Python and C++) based on the specification defined by desc.
        params = dict()
        params["num_action"] = GC.get_num_actions()
        params["num_group"] = 1 if args.actor_only else 2
        params["action_batchsize"] = desc["actor"]["input"]["batchsize"]
        if not args.actor_only:
            params["train_batchsize"] = desc["train"]["input"]["batchsize"]
        params["hist_len"] = args.hist_len
        params["T"] = args.T

        return GCWrapper(GC, co, desc, use_numpy=False, params=params)

cmd_line = "--num_games 64 --batchsize 16 --hist_len 1 --frame_skip 4 --actor_only"

nIter = 5000
elapsed_wait_only = 0

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    loader = Loader()
    args = ArgsProvider.Load(parser, [loader], cmd_line=cmd_line.split(" "))

    GC = loader.initialize()

    def actor(sel, sel_gpu):
        # pickle.dump(to_numpy(sel), open("tmp%d.bin" % k, "wb"), protocol=2)
        batchsize = sel["s"].size(0)
        actions = torch.LongTensor(batchsize)
        actions.zero_()
        return dict(a=actions)

    GC.reg_callback("actor", actor)

    reward_dist = Counter()

    before = datetime.now()
    GC.Start()

    import tqdm
    for k in tqdm.trange(nIter):
        b = datetime.now()
        # print("Before wait")
        GC.Run()
        # print("wake up from wait")
        elapsed_wait_only += (datetime.now() - b).total_seconds() * 1000

    print(reward_dist)
    elapsed = (datetime.now() - before).total_seconds() * 1000
    print("elapsed = %.4lf ms, elapsed_wait_only = %.4lf" % (elapsed, elapsed_wait_only))
    GC.PrintSummary()
    GC.Stop()

    # Compute the statistics.
    per_loop = elapsed / nIter
    per_wait = elapsed_wait_only / nIter
    per_frame_loop_n_cpu = per_loop / args.batchsize
    per_frame_wait_n_cpu = per_wait / args.batchsize

    fps_loop = 1000 / per_frame_loop_n_cpu * args.frame_skip
    fps_wait = 1000 / per_frame_wait_n_cpu * args.frame_skip

    print("Time[Loop]: %.6lf ms / loop, %.6lf ms / frame_loop_n_cpu, %.2f FPS" % (per_loop, per_frame_loop_n_cpu, fps_loop))
    print("Time[Wait]: %.6lf ms / wait, %.6lf ms / frame_wait_n_cpu, %.2f FPS" % (per_wait, per_frame_wait_n_cpu, fps_wait))
