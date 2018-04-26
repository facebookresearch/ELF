# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

#!/usr/bin/env python
# -*- coding: utf-8 -*-
# File: game.py

import argparse
import atari_game as atari
from collections import Counter
from datetime import datetime
import os
import random
import sys

# elf related import
sys.path.append('../')
from elf import GCWrapper, ContextArgs
from rlpytorch import ArgsProvider


class Loader:
    def __init__(self):
        self.context_args = ContextArgs()

        self.args = ArgsProvider(
            call_from=self,
            define_args=[
                ("frame_skip", 4),
                ("hist_len", 4),
                ("rom_file", "pong.bin"),
                ("actor_only", dict(action="store_true")),
                ("reward_clip", 1),
                ("rom_dir", os.path.dirname(__file__)),
                ("additional_labels", dict(
                    type=str,
                    default=None,
                    help="Add additional labels in the batch."
                    " E.g., id,seq,last_terminal",
                )),
                ("gpu", dict(type=int, default=None)),
            ],
            more_args=["batchsize", "T", "env_eval_only"],
            child_providers=[self.context_args.args])

    def initialize(self):
        args = self.args
        co = atari.ContextOptions()
        self.context_args.initialize(co)

        opt = atari.GameOptions()
        opt.frame_skip = args.frame_skip
        opt.rom_file = os.path.join(args.rom_dir, args.rom_file)
        opt.seed = 42
        opt.eval_only = getattr(args, "env_eval_only", 0) == 1
        opt.hist_len = args.hist_len
        opt.reward_clip = args.reward_clip

        GC = atari.GameContext(co, opt)
        print("Version: ", GC.Version())

        params = GC.GetParams()
        print("Num Actions: ", params["num_action"])

        desc = {}
        # For actor model, No reward needed, we only want to get input
        # and return distribution of actions.
        # sampled action and and value will be filled from the reply.

        desc["actor"] = dict(
            batchsize=args.batchsize,
            input=dict(T=1, keys=set(["s", "last_r", "last_terminal"])),
            reply=dict(T=1, keys=set(["rv", "pi", "V", "a"])))

        if not args.actor_only:
            # For training: group 1
            # We want input, action (filled by actor models), value
            # (filled by actor models) and reward.
            desc["train"] = dict(
                batchsize=args.batchsize,
                input=dict(
                    T=args.T,
                    keys=set([
                        "rv", "id", "pi", "s", "a", "last_r", "V", "seq",
                        "last_terminal"
                    ])),
                reply=None)

        if args.additional_labels is not None:
            extra = args.additional_labels.split(",")
            for _, v in desc.items():
                v["input"]["keys"].update(extra)

        # Initialize shared memory (between Python and C++)
        # based on the specification defined by desc.
        params["num_group"] = 1 if args.actor_only else 2
        params["action_batchsize"] = desc["actor"]["batchsize"]
        if not args.actor_only:
            params["train_batchsize"] = desc["train"]["batchsize"]
        params["hist_len"] = args.hist_len
        params["T"] = args.T

        return GCWrapper(
            GC, co, desc, gpu=args.gpu, use_numpy=False, params=params)


cmd_line = \
    "--num_games 64 --batchsize 16 --hist_len 1 --frame_skip 4 --actor_only"

nIter = 5000
elapsed_wait_only = 0

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    if len(sys.argv) > 1:
        cmd_line = sys.argv[1:]
    else:
        cmd_line = cmd_line.split(" ")

    loader = Loader()
    args = ArgsProvider.Load(parser, [loader], cmd_line=cmd_line)

    GC = loader.initialize()

    actor_count = 0
    train_count = 0

    def actor(batch):
        global actor_count, GC
        actor_count += 1
        batchsize = batch["s"].size(1)
        actions = [
            random.randint(0, GC.params["num_action"] - 1)
            for i in range(batchsize)
        ]
        reply = dict(a=actions)
        '''
        data = batch.to_numpy()
        data.update(reply)
        pickle.dump(
            data, open("tmp-actor%d.bin" % actor_count, "wb"), protocol=2)
        '''
        return reply

    def train(batch):
        global train_count
        # pickle.dump(
        #     sel.to_numpy(),
        #     open("tmp-train%d.bin" % train_count, "wb"),
        #     protocol=2, )
        train_count += 1

    GC.reg_callback("actor", actor)
    GC.reg_callback("train", train)

    reward_dist = Counter()

    before = datetime.now()
    GC.Start()

    import tqdm
    for _ in tqdm.trange(nIter):
        b = datetime.now()
        # Before wait
        GC.Run()
        # wake up from wait
        elapsed_wait_only += (datetime.now() - b).total_seconds() * 1000

    print(reward_dist)
    elapsed = (datetime.now() - before).total_seconds() * 1000
    print("elapsed = %.4lf ms, elapsed_wait_only = %.4lf" %
          (elapsed, elapsed_wait_only))
    GC.PrintSummary()
    GC.Stop()

    # Compute the statistics.
    per_loop = elapsed / nIter
    per_wait = elapsed_wait_only / nIter
    per_frame_loop_n_cpu = per_loop / args.batchsize
    per_frame_wait_n_cpu = per_wait / args.batchsize

    fps_loop = 1000 / per_frame_loop_n_cpu * args.frame_skip
    fps_wait = 1000 / per_frame_wait_n_cpu * args.frame_skip

    print(
        "Time[Loop]: %.6lf ms / loop, %.6lf ms / frame_loop_n_cpu, %.2f FPS" %
        (per_loop, per_frame_loop_n_cpu, fps_loop))
    print(
        "Time[Wait]: %.6lf ms / wait, %.6lf ms / frame_wait_n_cpu, %.2f FPS" %
        (per_wait, per_frame_wait_n_cpu, fps_wait))
