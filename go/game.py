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
                ("board_size", 19),
                ("num_planes", 25),
                ("num_future_actions", 3),
                ("actor_only", dict(action="store_true"))
            ],
            more_args = ["batchsize", "T"],
            child_providers = [ self.context_args.args ]
        )

    def initialize(self):
        args = self.args
        co = go.ContextOptions()
        self.context_args.initialize(co)

        opt = go.Options()
        opt.seed = 0
        opt.list_filename = "./train.lst"
        GC = go.GameContext(co, opt)
        print("Version: ", GC.Version())

        num_action = GC.get_num_actions()
        print("Num Actions: ", num_action)

        desc = {}
        # For actor model: group 0
        # No reward needed, we only want to get input and return distribution of actions.
        # sampled action and and value will be filled from the reply.

        # Descriptor of each group.
        #     desc = [(input_group0, reply_group0), (input_group1, reply_group1), ...]
        # GC.Wait(0) will return a batch of game states in the same group.
        # For example, if you register group 0 as the actor group, which has the history length of 1, and group 1 as the optimizer group which has the history length of T
        # Then you can check the group id to decide which Python procedure to use to deal with the group, by checking their group_id.
        # For self-play, we can register one group each player.
        desc["actor"] = (
            dict(id="", s="",  actions="", last_r="", last_terminal="", _batchsize=str(args.batchsize), _T="1"),
            dict(rv="", pi="", V="1", a="1", _batchsize=str(args.batchsize), _T="1")
        )

        if not args.actor_only:
            # For training: group 1
            # We want input, action (filled by actor models), value (filled by actor
            # models) and reward.
            desc["train"] = (
                dict(rv="", id="", pi="", actions="", s="", a="1", r="1", V="1", seq="", terminal="", _batchsize=str(args.batchsize), _T=str(args.T)),
                None
            )
        params = dict()
        params["num_action"] = GC.get_num_actions()
        params["board_size"] = args.board_size
        params["num_planes"] = args.num_planes
        params["num_future_actions"] = args.num_future_actions
        params["num_group"] = 1 if args.actor_only else 2
        params["action_batchsize"] = int(desc["actor"][0]["_batchsize"])
        if not args.actor_only:
            params["train_batchsize"] = int(desc["train"][0]["_batchsize"])
        #params["hist_len"] = args.hist_len
        params["T"] = args.T

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
