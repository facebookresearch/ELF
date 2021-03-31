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

from rlpytorch import *

if __name__ == '__main__':
    # import pdb
    # pdb.set_trace()
    trainer = Trainer()
    runner = SingleProcessRun()
    '''
        设置环境参数
            env : dice [
                game = game game.py
                sampler = sampler
                model_loaders =  存ModelLoader类的字典，每一个ModelLoader中有一个model
                mi=mi
            ]

            all_args: 所有定义的参数
    '''
    env, all_args = load_env(os.environ, trainer=trainer, runner=runner)  
    
    '''
       GC = GCWrapper(GC, co, desc, gpu=args.gpu, use_numpy=False, params=params)
          GC      /gameMC/python_wrapper.cc GameContext
          co      /elf/python_options_utils_cpp.h ContextOption
          desc    actor 和 critic 的 Batch定义  
          {'actor': {'batchsize': 64, 'input': {'T': 1, 'keys': {'s', 'terminal', 'last_r'}}, 'reply': {'T': 1, 'keys': {'a', 'pi', 'V'}}}}
    '''
    GC = env["game"].initialize()

    model = env["model_loaders"][0].load_model(GC.params)
    env["mi"].add_model("model", model, opt=True)
    env["mi"].add_model("actor", model, copy=True, cuda=all_args.gpu is not None, gpu_id=all_args.gpu)

    trainer.setup(sampler=env["sampler"], mi=env["mi"], rl_method=env["method"])

    GC.reg_callback("train", trainer.train)
    GC.reg_callback("actor", trainer.actor)

    runner.setup(GC, episode_summary=trainer.episode_summary,
                episode_start=trainer.episode_start)

    runner.run()

