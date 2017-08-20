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
            call_from = self,
            define_args = [
                ("handicap_level", 0),
                ("latest_start", 1000),
                ("latest_start_decay", 0.7),
                ("players", dict(type=str, help=";-separated player infos. For example: type=AI_NN,fs=50,backup=AI_SIMPLE,fow=True;type=AI_SIMPLE,fs=50")),
                ("max_tick", dict(type=int, default=30000, help="Maximal tick")),
                ("mcts_threads", 64),
                ("seed", 0),
                ("simple_ratio", -1),
                ("ratio_change", 0),
                ("actor_only", dict(action="store_true")),
                ("additional_labels", dict(type=str, default=None, help="Add additional labels in the batch. E.g., id,seq,last_terminal")),
                ("model_no_spatial", dict(action="store_true")) # TODO, put it to model
            ],
            more_args = ["batchsize", "T"],
            child_providers = [ self.context_args.args ]
        )

    def _set_key(self, ai_options, key, value):
        if not hasattr(ai_options, key):
            print("AIOptions does not have key = " + key)
            return

        # Can we automate this?
        type_convert = dict(ai_nn=minirts.AI_NN, ai_simple=minirts.AI_SIMPLE, ai_hit_and_run=minirts.AI_HIT_AND_RUN)
        bool_convert = dict(t=True, true=True, f=False, false=False)

        if key == "type" or key == "backup":
            setattr(ai_options, key, type_convert[value.lower()])
        elif key == "fow":
            setattr(ai_options, key, bool_convert[value.lower()])
        else:
            setattr(ai_options, key, int(value))

    def _parse_players(self, opt):
        for player in self.args.players.split(";"):
            ai_options = minirts.AIOptions()
            for item in player.split(","):
                key, value = item.split("=")
                self._set_key(ai_options, key, value)
            opt.AddAIOptions(ai_options)

    def _init_gc(self):
        args = self.args

        co = minirts.ContextOptions()
        self.context_args.initialize(co)

        opt = minirts.Options()
        opt.seed = args.seed
        opt.simulation_type = minirts.ST_NORMAL
        opt.latest_start = args.latest_start
        opt.latest_start_decay = args.latest_start_decay
        opt.mcts_threads = args.mcts_threads
        opt.mcts_rollout_per_thread = 50
        opt.max_tick = args.max_tick
        opt.handicap_level = args.handicap_level
        opt.simple_ratio = args.simple_ratio
        opt.ratio_change = args.ratio_change

        self._parse_players(opt)

        # opt.output_filename = b"simulators.txt"
        # opt.output_filename = b"cout"
        # opt.cmd_dumper_prefix = b"cmd-dump"
        # opt.save_replay_prefix = b"replay"

        GC = minirts.GameContext(co, opt)
        params = GC.GetParams()
        print("Version: ", GC.Version())

        print("Num Actions: ", params["num_action"])
        print("Num unittype: ", params["num_unit_type"])

        return co, GC, params

    def _add_more_labels(self, desc):
        args = self.args
        if args.additional_labels is None: return

        extra = args.additional_labels.split(",")
        for _, v in desc.items():
            v["input"]["keys"].update(extra)

    def _add_player_id(self, desc, player_id):
        desc["input"]["keys"].add("player_id")
        desc["filters"] = dict(player_id=player_id)

    def _get_actor_spec(self):
        return dict(
            batchsize=self.args.batchsize,
            input=dict(T=1, keys=set(["s", "res", "last_r", "terminal"])),
            reply=dict(T=1, keys=set(["rv", "pi", "V", "a"]))
        )

    def _get_train_spec(self):
        return dict(
            batchsize=self.args.batchsize,
            input=dict(T=self.args.T, keys=set(["rv", "pi", "s", "res", "a", "last_r", "V", "terminal"])),
            reply=None
        )

    def initialize(self):
        co, GC, params = self._init_gc()
        args = self.args

        desc = {}
        # For actor model, no reward needed, we only want to get input and return distribution of actions.
        # sampled action and and value will be filled from the reply.
        desc["actor"] = self._get_actor_spec()

        if not args.actor_only:
            # For training, we want input, action (filled by actor models), value (filled by actor models) and reward.
            desc["train"] = self._get_train_spec()

        self._add_more_labels(desc)

        params.update(dict(
            num_group = 1 if args.actor_only else 2,
            action_batchsize = int(desc["actor"]["batchsize"]),
            train_batchsize = int(desc["train"]["batchsize"]) if not args.actor_only else None,
            T = args.T,
            model_no_spatial = args.model_no_spatial
        ))

        return GCWrapper(GC, co, desc, use_numpy=False, gpu=None, params=params)

    def initialize_selfplay(self):
        args = self.args
        args.ai_type = "AI_NN"
        args.opponent_type = "AI_NN"
        co, GC, params = self._init_gc()

        desc = {}
        # For actor model, no reward needed, we only want to get input and return distribution of actions.
        # sampled action and and value will be filled from the reply.
        desc["actor0"] = self._get_actor_spec()
        desc["actor1"] = self._get_actor_spec()

        self._add_player_id(desc["actor0"], "reference")
        self._add_player_id(desc["actor1"], "train")

        if not args.actor_only:
            # For training, we want input, action (filled by actor models), value (filled by actor models) and reward.
            desc["train1"] = self._get_train_spec()
            self._add_player_id(desc["train1"], "train")

        self._add_more_labels(desc)

        params.update(dict(
            num_group = 1 if args.actor_only else 2,
            action_batchsize = int(desc["actor0"]["batchsize"]),
            train_batchsize = int(desc["train1"]["batchsize"]) if not args.actor_only else None,
            T = args.T,
            model_no_spatial = args.model_no_spatial
        ))

        return GCWrapper(GC, co, desc, use_numpy=False, params=params)

nIter = 5000
elapsed_wait_only = 0

import pickle
import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    loader = Loader()
    args = ArgsProvider.Load(parser, [loader])

    def actor(sel, sel_gpu):
        '''
        import pdb
        pdb.set_trace()
        pickle.dump(utils_elf.to_numpy(sel), open("tmp%d.bin" % k, "wb"), protocol=2)
        '''
        return dict(a=[0]*sel["s"].size(1))

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
