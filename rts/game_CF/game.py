# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import minirts
from datetime import datetime

import sys, os
from elf import GCWrapper, ContextArgs
from rlpytorch import ArgsProvider

class Loader:
    def __init__(self):
        self.context_args = ContextArgs()

        self.args = ArgsProvider(
            define_args = [
                ("handicap_level", 0),
                ("latest_start", 1000),
                ("latest_start_decay", 0.7),
                ("fs_ai", 50),
                ("fs_opponent", 50),
                ("ai_type", dict(type=str, choices=["AI_SIMPLE", "AI_HIT_AND_RUN", "AI_NN", "AI_FLAG_NN", "AI_TD_NN"], default="AI_NN")),
                ("opponent_type", dict(type=str, choices=["AI_SIMPLE", "AI_HIT_AND_RUN", "AI_FLAG_SIMPLE", "AI_TD_BUILT_IN"], default="AI_SIMPLE")),
                ("max_tick", dict(type=int, default=30000, help="Maximal tick")),
                ("mcts_threads", 64),
                ("seed", 0),
                ("simple_ratio", -1),
                ("ratio_change", 0),
                ("actor_only", dict(action="store_true"))
            ],
            more_args = ["batchsize", "T"],
            child_providers = [ self.context_args.args ]
        )

    def initialize(self):
        args = self.args

        co = minirts.ContextOptions()
        self.context_args.initialize(co)

        opt = minirts.Options()
        opt.seed = args.seed
        opt.frame_skip_ai = args.fs_ai
        opt.frame_skip_opponent = args.fs_opponent
        opt.simulation_type = minirts.ST_NORMAL
        opt.ai_type = getattr(minirts, args.ai_type)
        if args.ai_type == "AI_NN":
            opt.backup_ai_type = minirts.AI_SIMPLE
        if args.ai_type == "AI_FLAG_NN":
            opt.backup_ai_type = minirts.AI_FLAG_SIMPLE
        opt.opponent_ai_type = getattr(minirts, args.opponent_type)
        opt.latest_start = args.latest_start
        opt.latest_start_decay = args.latest_start_decay
        opt.mcts_threads = args.mcts_threads
        opt.mcts_rollout_per_thread = 50
        opt.max_tick = args.max_tick
        opt.handicap_level = args.handicap_level
        opt.simple_ratio = args.simple_ratio
        opt.ratio_change = args.ratio_change
        # opt.output_filename = b"simulators.txt"
        # opt.cmd_dumper_prefix = b"cmd-dump"
        # opt.save_replay_prefix = b"replay"

        GC = minirts.GameContext(co, opt)
        print("Version: ", GC.Version())

        num_action = GC.get_num_actions()
        print("Num Actions: ", num_action)

        num_unittype = GC.get_num_unittype()
        print("Num unittype: ", num_unittype)

        desc = {}
        # For actor model, no reward needed, we only want to get input and return distribution of actions.
        # sampled action and and value will be filled from the reply.
        desc["actor"] = dict(
            input=dict(id="", s=str(num_unittype+8), r0="", r1="", last_r="", last_terminal="", _batchsize=str(args.batchsize), _T="1"),
            reply=dict(rv="", pi=str(num_action), V="1", a="1", _batchsize=str(args.batchsize), _T="1")
        )

        if not args.actor_only:
            # For training: group 1
            # We want input, action (filled by actor models), value (filled by actor
            # models) and reward.
            desc["train"] = dict(
                input=dict(rv="", id="", pi=str(num_action), s=str(num_unittype+8),
                           r0="", r1="", a="1", r="1", V="1", seq="", terminal="",
                           _batchsize=str(args.batchsize), _T=str(args.T)),
                reply=None
            )

        params = dict(
            num_action = num_action,
            num_unit_type = num_unittype,
            num_group = 1 if args.actor_only else 2,
            action_batchsize = int(desc["actor"]["input"]["_batchsize"]),
            train_batchsize = int(desc["train"]["input"]["_batchsize"]) if not args.actor_only else None,
            T = args.T
        )

        return GCWrapper(GC, co, desc, use_numpy=False, params=params)

nIter = 5000
elapsed_wait_only = 0

import pickle
import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    loader = Loader()
    args = ArgsProvider.Load(parser, [loader])

    def actor(sel, sel_gpu, reply):
        '''
        import pdb
        pdb.set_trace()
        pickle.dump(utils_elf.to_numpy(sel), open("tmp%d.bin" % k, "wb"), protocol=2)
        '''
        reply[0]["a"][:] = 0

    GC = loader.initialize()
    GC.reg_callback("actor", actor)

    before = datetime.now()
    GC.Start()

    import tqdm
    for k in tqdm.trange(nIter):
        b = datetime.now()
        GC.Run()
        elapsed_wait_only += (datetime.now() - b).total_seconds() * 1000
        #img = np.array(infos[0].data.image, copy=False)

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
