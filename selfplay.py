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

    sampler = Sampler()
    trainer = Trainer()
    game = load_module(os.environ["game"]).Loader()
    runner = SingleProcessRun()
    model_file = load_module(os.environ["model_file"])
    model_class, method_class = model_file.Models[os.environ["model"]]

    model_loader = ModelLoader(model_class)
    method = method_class()

    args_providers = [sampler, trainer, game, runner, model_loader, method]

    all_args = ArgsProvider.Load(parser, args_providers)

    GC = game.initialize_selfplay()
    GC.setup_gpu(all_args.gpu)
    all_args.method_class = method_class

    model = model_loader.load_model(GC.params)
    mi = ModelInterface()
    mi.add_model("model", model, optim_params={ "lr" : 0.001})
    mi.add_model("actor", model, copy=True, cuda=True, gpu_id=all_args.gpu)
    method.set_model_interface(mi)

    trainer.setup(sampler=sampler, mi=mi, rl_method=method)

    evaluator = Evaluator()
    evalator.set(all_args)
    evaluator.setup(sampler=sampler, mi=mi.clone())

    GC.reg_callback("train0", trainer.train)
    GC.reg_callback("actor0", trainer.actor)
    GC.reg_callback("actor1", evaluator.actor)

    def summary(i):
        trainer.episode_summary(i)
        evaluator.episode_summary(i)

    def start():
        trainer.episode_start()
        evaluator.episode_start()

    runner.setup(GC, episode_summary=summary, episode_start=start)
    runner.run()

