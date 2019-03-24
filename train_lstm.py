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

from rlpytorch import LSTMTrainer, Sampler, SingleProcessRun, load_env, ModelLoader, ArgsProvider, ModelInterface

if __name__ == '__main__':
    trainer = LSTMTrainer()
    runner = SingleProcessRun()
    env, all_args = load_env(os.environ, trainer=trainer, runner=runner)

    GC = env["game"].initialize()

    model = env["model_loaders"][0].load_model(GC.params)
    mi = ModelInterface()
    mi.add_model("model", model, params={ "lr" : 0.001})
    mi.add_model("actor", model, copy=True, cuda=all_args.gpu is not None, gpu_id=all_args.gpu)

    trainer.setup(sampler=env["sampler"], mi=mi, rl_method=env["method"])

    GC.reg_callback("train", trainer.train)
    GC.reg_callback("actor", trainer.actor)
    runner.setup(GC, episode_summary=trainer.episode_summary,
                episode_start=trainer.episode_start)

    runner.run()

