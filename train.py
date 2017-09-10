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
    runner = SingleProcessRun()
    game, method, model_loaders = load_env(os.environ)

    args_providers = [sampler, trainer, game, runner, method] + model_loaders
    all_args = ArgsProvider.Load(parser, args_providers)

    GC = game.initialize()

    model = model_loaders[0].load_model(GC.params)
    mi = ModelInterface()
    mi.add_model("model", model, optim_params={ "lr" : 0.001})
    mi.add_model("actor", model, copy=True, cuda=all_args.gpu is not None, gpu_id=all_args.gpu)

    trainer.setup(sampler=sampler, mi=mi, rl_method=method)

    GC.reg_callback("train", trainer.train)
    GC.reg_callback("actor", trainer.actor)
    runner.setup(GC, episode_summary=trainer.episode_summary,
                episode_start=trainer.episode_start)

    runner.run()

