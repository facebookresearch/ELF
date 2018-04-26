# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import os
import sys

from rlpytorch import ArgsProvider

class ContextArgs:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("num_games", 1024),
                ("batchsize", 128),
                ("game_multi", dict(type=int, default=None)),
                ("T", 6),
                ("eval", dict(action="store_true")),
                ("wait_per_group", dict(action="store_true")),
                ("num_collectors", 0),
                ("verbose_comm", dict(action="store_true")),
                ("verbose_collector", dict(action="store_true")),
                ("mcts_threads", 0),
                ("mcts_rollout_per_thread", 1),
                ("mcts_verbose", dict(action="store_true")),
                ("mcts_save_tree_filename", ""),
                ("mcts_verbose_time", dict(action="store_true")),

                ("mcts_use_prior", dict(action="store_true")),
                ("mcts_pseudo_games", 0),
                ("mcts_pick_method", "most_visited"),
            ],
            on_get_args = self._on_get_args
        )

    def _on_get_args(self, args):
        if args.eval:
            args.num_games = 1
            args.batchsize = 1
        if args.game_multi is not None:
            args.num_games = args.batchsize * args.game_multi

    def initialize(self, co):
        args = self.args
        if args.game_multi is not None:
            args.num_games = args.batchsize * args.game_multi

        co.num_games = args.num_games
        co.T = args.T
        co.wait_per_group = args.wait_per_group
        co.verbose_comm = args.verbose_comm
        co.verbose_collector = args.verbose_collector

        co.max_num_threads = args.mcts_threads
        co.num_collectors = args.num_collectors

        mcts = co.mcts_options

        mcts.num_threads = args.mcts_threads
        mcts.num_rollout_per_thread = args.mcts_rollout_per_thread
        mcts.verbose = args.mcts_verbose
        mcts.verbose_time = args.mcts_verbose_time
        mcts.save_tree_filename = args.mcts_save_tree_filename
        mcts.use_prior = args.mcts_use_prior
        mcts.pseudo_games = args.mcts_pseudo_games
        mcts.pick_method = args.mcts_pick_method


