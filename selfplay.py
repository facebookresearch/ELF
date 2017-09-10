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

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    verbose = False

    sampler = Sampler()
    trainer = Trainer(verbose=verbose)
    runner = SingleProcessRun()
    evaluator = Evaluator(stats=False, verbose=verbose)

    game, method, model_loaders = load_env(os.environ)

    args_providers = [sampler, trainer, game, runner, method, evaluator] + model_loaders
    all_args = ArgsProvider.Load(parser, args_providers)

    GC = game.initialize_selfplay()

    model = model_loaders[0].load_model(GC.params)
    mi = ModelInterface()
    mi.add_model("model", model, copy=False, optim_params={ "lr" : 0.001})
    mi.add_model("actor", model, copy=True, cuda=all_args.gpu is not None, gpu_id=all_args.gpu)

    trainer.setup(sampler=sampler, mi=mi, rl_method=method)
    evaluator.setup(sampler=sampler, mi=mi.clone(gpu=all_args.gpu))

    if not all_args.actor_only:
        GC.reg_callback("train1", trainer.train)
    GC.reg_callback("actor1", trainer.actor)
    GC.reg_callback("actor0", evaluator.actor)

    def summary(i):
        trainer.episode_summary(i)
        evaluator.episode_summary(i)

    def start(i):
        trainer.episode_start(i)
        evaluator.episode_start(i)

    runner.setup(GC, episode_summary=summary, episode_start=start)
    runner.run()

