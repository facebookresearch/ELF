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
from elf import GCWrapper, ContextArgs, MoreLabels
from rlpytorch import ArgsProvider

class Loader:
    def __init__(self):
        self.context_args = ContextArgs()
        self.more_labels = MoreLabels()

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("actor_only", dict(action="store_true")),
                ("list_file", "./train.lst"),
                ("verbose", dict(action="store_true")),
                ("data_aug", dict(type=int, default=-1, help="specify data augumentation, 0-7, -1 mean random")),
                ("ratio_pre_moves", dict(type=float, default=0.5, help="how many moves to perform before we train the model")),
                ("move_cutoff", dict(type=int, default=-1, help="Cutoff ply in replay")),
                ("online", dict(action="store_true", help="Set game to online mode")),
                ("gpu", dict(type=int, default=None))
            ],
            more_args = ["batchsize", "T"],
            child_providers = [ self.context_args.args, self.more_labels.args ]
        )

    def initialize(self):
        args = self.args
        co = go.ContextOptions()
        self.context_args.initialize(co)

        opt = go.GameOptions()
        opt.seed = 0
        opt.list_filename = args.list_file
        opt.online = args.online
        opt.verbose = args.verbose
        opt.data_aug = args.data_aug
        opt.ratio_pre_moves = args.ratio_pre_moves
        opt.move_cutoff = args.move_cutoff
        GC = go.GameContext(co, opt)
        print("Version: ", GC.Version())

        params = GC.GetParams()
        print("Num Actions: ", params["num_action"])

        desc = {}
        if args.online:
            desc["actor"] = dict(
                batchsize=args.batchsize,
                input=dict(T=args.T, keys=set(["s"])),
                reply=dict(T=args.T, keys=set(["V", "a"]))
            )
        else:
            desc["train"] = dict(
                batchsize=args.batchsize,
                input=dict(T=args.T, keys=set(["s", "offline_a"])),
                reply=None
            )

        self.more_labels.add_labels(desc)

        params.update(dict(
            num_group = 1 if args.actor_only else 2,
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
