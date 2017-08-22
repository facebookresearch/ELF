# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
from datetime import datetime

import sys
import os

from rlpytorch import *

class Eval:
    def __init__(self):
        self.stats = Stats("eval")

        self.model_file = load_module(os.environ["model_file"])
        model_class, method_class = self.model_file.Models[os.environ["model"]]
        self.model_loader = ModelLoader(model_class)

        self.game = load_module(os.environ["game"]).Loader()
        self.game.args.set_override(actor_only=True, game_multi=2)
        self.sampler = Sampler()
        self.evaluator = Evaluator(stats=False)

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("num_eval", 500),
                ("tqdm", dict(action="store_true"))
            ],
            more_args = [ "gpu" ],
            child_providers = [
                self.stats.args, self.game.args,
                self.sampler.args, self.evaluator.args,
                self.model_loader.args
            ]
        )

    def run(self):
        self.GC = self.game.initialize()
        self.GC.setup_gpu(self.args.gpu)

        self.stats.reset()

        model = self.model_loader.load_model(self.GC.params)
        mi = ModelInterface()
        mi.add_model("model", model, optim_params={ "lr" : 0.001})
        mi.add_model("actor", model, copy=True, cuda=True, gpu_id=self.args.gpu)

        def actor(sel, sel_gpu):
            reply = self.evaluator.actor(sel, sel_gpu)
            '''
            s = sel["s"][0][0]
            seq = sel["seq"][0][0]
            for i in range(s.size(0)):
                print("[seq=%d][c=%d]: %s" % (seq, i, str(s[i])))
            print("[seq=%d]: %s" % (seq, str(reply["pi"][0])))
            print("[seq=%d]: %s" % (seq, str(reply["a"][0])))
            '''
            self.stats.feed_batch(sel)
            return reply

        self.GC.reg_callback("actor", actor)
        self.GC.Start()

        self.evaluator.setup(sampler=self.sampler, mi=mi)
        self.evaluator.episode_start(0)

        if self.args.tqdm:
            import tqdm
            tq = tqdm.tqdm(total=self.args.num_eval)
            while self.stats.count_completed() < self.args.num_eval:
                old_n = self.stats.count_completed()
                self.GC.Run()
                diff = self.stats.count_completed() - old_n
                tq.update(diff)
            tq.close()
        else:
            while self.stats.count_completed() < self.args.num_eval:
                self.GC.Run()

        self.stats.print_summary()
        self.GC.Stop()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    evaluator = Eval()
    args = ArgsProvider.Load(parser, [ evaluator ])

    evaluator.run()

