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
                ("ratio_pre_moves", dict(type=float, default=0, help="how many moves to perform in each thread, before we use the data train the model")),
                ("start_ratio_pre_moves", dict(type=float, default=0.5, help="how many moves to perform in each thread, before we use the first sgf file to train the model")),
                ("num_games_per_thread", dict(type=int, default=5, help="number of concurrent games per threads, used to increase diversity of games")),
                ("move_cutoff", dict(type=int, default=-1, help="Cutoff ply in replay")),
                ("online", dict(action="store_true", help="Set game to online mode")),
                ("use_mcts", dict(action="store_true")),
                ("gpu", dict(type=int, default=None))
            ],
            more_args = ["batchsize", "T"],
            child_providers = [ self.context_args.args, self.more_labels.args ]
        )

    def initialize(self):
        args = self.args
        co = go.ContextOptions()
        self.context_args.initialize(co)
        co.print()

        opt = go.GameOptions()
        opt.seed = 0
        opt.list_filename = args.list_file
        opt.online = args.online
        opt.use_mcts = args.use_mcts
        opt.verbose = args.verbose
        opt.data_aug = args.data_aug
        opt.ratio_pre_moves = args.ratio_pre_moves
        opt.start_ratio_pre_moves = args.start_ratio_pre_moves
        opt.move_cutoff = args.move_cutoff
        opt.num_games_per_thread = args.num_games_per_thread
        GC = go.GameContext(co, opt)
        print("Version: ", GC.Version())

        params = GC.GetParams()
        print("Num Actions: ", params["num_action"])

        desc = {}
        if args.online:
            desc["actor"] = dict(
                batchsize=args.batchsize,
                input=dict(T=args.T, keys=set(["s"])),
                reply=dict(T=args.T, keys=set(["V", "pi", "a"]))
            )
            desc["mcts_actor"] = dict(
                batchsize=args.batchsize,
                input=dict(T=1, keys=set(["s"])),
                reply=dict(T=1, keys=set(["V", "pi", "a"])),
                name="mcts_actor",
                timeout_usec = 100
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
    args = ArgsProvider.Load(parser, [loader], global_overrides=dict(additional_labels="move_idx,game_record_idx"))

    GC = loader.initialize()

    import torch
    nbin = 10
    stats = torch.FloatTensor(nbin, 19, 19)
    counts = torch.FloatTensor(10)

    game_records_visited = Counter()

    our_idx = GC.params["our_stone_plane"]
    opp_idx = GC.params["opponent_stone_plane"]

    def train(batch):
        # Collect statistics.
        b = batch.hist(0)
        for game_idx, move_idx, s in zip(b["game_record_idx"], b["move_idx"], b["s"]):
            bin_idx =  move_idx // 10
            if bin_idx >= nbin: continue
            game_records_visited[game_idx] += 1
            stats[bin_idx, :, :] += s[our_idx, :, :]
            stats[bin_idx, :, :] += s[opp_idx, :, :]
            counts[bin_idx] += 1

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

    print(len(game_records_visited))
    print(game_records_visited.most_common(30))

    stats /= stats.sum(1).sum(1).view(-1, 1, 1)
    for i in range(nbin):
        print("Range [%d, %d)" % (i * 10, i * 10 + 10))
        print(stats[i, :, :])

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
