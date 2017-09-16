#!/usr/bin/env python
# -*- coding: utf-8 -*-
# File: game.py

from datetime import datetime
from collections import Counter
import argparse
from time import sleep
import os
import go_game as go

import sys
sys.path.append(os.path.join(os.path.dirname(__file__), ".."))
from elf import GCWrapper, ContextArgs
from rlpytorch import ArgsProvider

class Loader:
    def __init__(self):
        self.context_args = ContextArgs()

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("actor_only", dict(action="store_true")),
                ("list_file", "./train.lst"),
                ("verbose", dict(action="store_true")),
                ("online", dict(action="store_true", default="Set game to online model, then the game will read the actions and proceed")),
                ("gpu", dict(type=int, default=None))
            ],
            more_args = ["batchsize", "T"],
            child_providers = [ self.context_args.args ]
        )

    def initialize(self):
        args = self.args
        co = go.ContextOptions()
        self.context_args.initialize(co)

        opt = go.GameOptions()
        opt.seed = 0
        opt.list_filename = args.list_file
        opt.verbose = args.verbose
        GC = go.GameContext(co, opt)
        print("Version: ", GC.Version())

        params = GC.GetParams()
        print("Num Actions: ", params["num_action"])

        desc = {}

        # For training: group 1
        # We want input, action (filled by actor models), value (filled by actor
        # models) and reward.
        desc["train"] = dict(
            batchsize=args.batchsize,
            input=dict(T=args.T, keys=set(["features", "offline_a"])),
            reply=None
        )

        desc["actor"] = dict(
            batchsize=args.batchsize,
            input=dict(T=args.T, keys=set(["features"])),
            reply=dict(T=args.T, keys=set(["V", "a"]))
        )

        params.update(dict(
            num_group = 1 if args.actor_only else 2,
            train_batchsize = int(desc["train"]["batchsize"]),
            T = args.T,
        ))

        return GCWrapper(GC, co, desc, gpu=args.gpu, use_numpy=False, params=params)

elapsed_wait_only = 0

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--num_iter", type=int, default=5000)

    loader = Loader()
    args = ArgsProvider.Load(parser, [loader])

    GC = loader.initialize()

    def train(batch):
        import pdb
        pdb.set_trace()

    GC.reg_callback("train", train)

    reward_dist = Counter()

    before = datetime.now()
    GC.Start()

    import tqdm
    for k in tqdm.trange(args.num_iter):
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
