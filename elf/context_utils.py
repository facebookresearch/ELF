# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'rlpytorch'))
from args_utils import ArgsProvider, args_loader

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
                ("verbose_comm", dict(action="store_true")),
                ("verbose_collector", dict(action="store_true"))
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

