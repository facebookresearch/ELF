# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

#!/usr/bin/env python
# -*- coding: utf-8 -*-

from datetime import datetime

import sys
import os

from rlpytorch import load_env, Evaluator, ArgsProvider, EvalIters

if __name__ == '__main__':
    evaluator = Evaluator(stats=False)
    eval_iters = EvalIters()
    env, args = load_env(os.environ, overrides=dict(actor_only=True), eval_iters=eval_iters, evaluator=evaluator)

    GC = env["game"].initialize_reduced_service()

    model = env["model_loaders"][0].load_model(GC.params)
    mi = env["mi"]
    mi.add_model("actor", model, cuda=args.gpu is not None, gpu_id=args.gpu)

    def reduced_project(batch):
        output = mi["actor"].forward(batch.hist(0))
        eval_iters.stats.feed_batch(batch)
        return dict(reduced_s=output["h"].data)

    def reduced_forward(batch):
        b = batch.hist(0)
        output = mi["actor"].transition(b["reduced_s"], b["a"])
        return dict(reduced_next_s=output["hf"].data)

    def reduced_predict(batch):
        b = batch.hist(0)
        output = mi["actor"].decision(b["reduced_s"])
        return dict(pi=output["pi"].data, V=output["V"].data)

    def actor(batch):
        return evaluator.actor(batch)

    evaluator.setup(mi=mi, sampler=env["sampler"])
    if GC.reg_has_callback("actor"):
        GC.reg_callback("actor", actor)

    GC.reg_callback("reduced_predict", reduced_predict)
    GC.reg_callback("reduced_forward", reduced_forward)
    GC.reg_callback("reduced_project", reduced_project)

    evaluator.episode_start(0)
    GC.Start()

    for _ in eval_iters.iters():
        GC.Run()
    GC.Stop()

