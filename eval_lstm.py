# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
from datetime import datetime

import sys
import os

from rlpytorch import LSTMTrainer, Sampler, EvalIters, load_env, ModelLoader, ArgsProvider, ModelInterface

if __name__ == '__main__':
    trainer = LSTMTrainer()
    eval_iters = EvalIters()
    env, all_args = load_env(os.environ, overrides=dict(actor_only=True), trainer=trainer, eval_iters=eval_iters)

    GC = env["game"].initialize()

    model = env["model_loaders"][0].load_model(GC.params)
    mi = ModelInterface()
    mi.add_model("model", model)
    mi.add_model("actor", model, copy=True, cuda=all_args.gpu is not None, gpu_id=all_args.gpu)

    trainer.setup(sampler=env["sampler"], mi=env["mi"])

    def actor(batch):
        reply = trainer.actor(batch)
        eval_iters.stats.feed_batch(batch)
        return reply

    GC.reg_callback("actor", actor)
    GC.Start()

    trainer.episode_start(0)

    for n in eval_iters.iters():
        GC.Run()

    GC.Stop()
