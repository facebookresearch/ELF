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
from elf import GCWrapper, ContextArgs
from rlpytorch import ArgsProvider

class Loader:
    def __init__(self):
        self.context_args = ContextArgs()

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("actor_only", dict(action="store_true"))
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
        opt.list_filename = "./go/train.lst"
        GC = go.GameContext(co, opt)
        print("Version: ", GC.Version())

        params = GC.GetParams()
        print("Num Actions: ", params["num_action"])

        desc = {}
        desc["actor"] = dict(
            batchsize=args.batchsize,
            input=dict(T=1, keys=set(["features", "actions"])),
            reply=dict(T=1, keys=set(["rv", "pi", "V", "a"]))
        )

        if not args.actor_only:
            # For training: group 1
            # We want input, action (filled by actor models), value (filled by actor
            # models) and reward.
            desc["train"] = dict(
                batchsize=args.batchsize,
                input=dict(T=args.T, keys=set(["rv", "pi", "features", "actions", "a", "V"])),
                reply=None
            )

        params.update(dict(
            num_group = 1 if args.actor_only else 2,
            action_batchsize = int(desc["actor"]["batchsize"]),
            train_batchsize = int(desc["train"]["batchsize"]) if not args.actor_only else None,
            T = args.T,
        ))

        return GCWrapper(GC, co, desc, use_numpy=False, params=params)

cmd_line = "--num_games 1 --batchsize 1 --actor_only"

nIter = 5000
elapsed_wait_only = 0

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    loader = Loader()
    args = ArgsProvider.Load(parser, [loader], cmd_line=cmd_line.split(" "))

    GC = loader.initialize()

    def actor(sel, sel_gpu, reply):
        # pickle.dump(to_numpy(sel), open("tmp%d.bin" % k, "wb"), protocol=2)
        reply[0]["a"][:] = 0

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
