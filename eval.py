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

from rlpytorch import ModelLoader, load_env, Sampler, Evaluator, ModelInterface, ArgsProvider, EvalIters

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    evaluator = Evaluator(stats=False)
    eval_iters = EvalIters()

    env = load_env(os.environ)
    env["game"].args.set_override(actor_only=True, game_multi=2)

    args = ArgsProvider.Load(parser, [ env, evaluator, eval_iters ])

    GC = env["game"].initialize()

    model = env["model_loaders"][0].load_model(GC.params)
    mi = ModelInterface()
    mi.add_model("model", model, optim_params={ "lr" : 0.001})
    mi.add_model("actor", model, copy=True, cuda=True, gpu_id=args.gpu)

    def actor(batch):
        reply = evaluator.actor(batch)
        '''
        s = batch["s"][0][0]
        seq = batch["seq"][0][0]
        for i in range(s.size(0)):
            print("[seq=%d][c=%d]: %s" % (seq, i, str(s[i])))
        print("[seq=%d]: %s" % (seq, str(reply["pi"][0])))
        print("[seq=%d]: %s" % (seq, str(reply["a"][0])))
        '''
        eval_iters.stats.feed_batch(batch)
        return reply

    evaluator.setup(sampler=env["sampler"], mi=mi)

    GC.reg_callback("actor", actor)
    GC.Start()

    evaluator.episode_start(0)

    for n in eval_iters.iters():
        GC.Run()
    GC.Stop()

