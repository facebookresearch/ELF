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

def remote_init(mi, gpu_id, args):
    rl_method = args.method_class(mi, args)
    # Local GPU copy of the model.
    if gpu_id != 0:
        mi_local = mi.clone(gpu=gpu_id)
        rl_method_local = args.method_class(mi_local, args)
        return dict(rl_method=rl_method, rl_method_local=rl_method_local,
                    mi=mi, mi_local=mi_local, count=0, gpu_id=gpu_id)
    else:
        return dict(rl_method=rl_method, mi=mi, count=0, gpu_id=gpu_id)

def remote_run(context, batch_gpu):
    rl_method = context["rl_method"]
    mi = context["mi"]

    if context["gpu_id"] == 0:
        rl_method.run(batch_gpu)
    else:
        # Not on the same gpu, so we first update on the local gpu.
        rl_method_local = context["rl_method_local"]
        mi_local = context["mi_local"]
        #rl_method_local.run(batch_gpu, update_params=False)
        rl_method_local.run(batch_gpu)

        mi.average_model("model", mi_local["model"])
        # Once a while you need to update the local model so that it catch up
        # with the global one.

        mi_local.update_model("model", mi["model"])

    # once a while you need to update the actor.
    mi.update_model("actor", mi["model"])

    context["count"] += 1
    if context["count"] % 500 == 0:
        if context["gpu_id"] == 0:
            rl_method.print_stats(context["count"] // 500)
        else:
            rl_method_local.print_stats(context["count"] // 500)

class Eval:
    def __init__(self):
        self.stats = Stats("eval")
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("num_eval", 500),
                ("eval_multinomial", dict(action="store_true"))
            ],
            child_providers = [ self.stats.args ]
        )

    def setup(self, all_args):
        self.game = load_module(os.environ["game"]).Loader()
        self.game.args.set(all_args, actor_only=True, game_multi=2)

        self.gpu = all_args.eval_gpu
        self.tqdm = all_args.tqdm

        self.runner = SingleProcessRun()
        self.runner.args.set(all_args)

        self.GC = self.game.initialize()
        self.GC.setup_gpu(self.gpu)

        self.sampler = Sampler()
        self.sampler.args.set(all_args, greedy=not self.args.eval_multinomial)

        self.trainer = Trainer()
        self.trainer.args.set(all_args)

        def actor(sel, sel_gpu):
            reply = self.trainer.actor(sel, sel_gpu)
            self.stats.feed_batch(sel)
            return reply

        self.GC.reg_callback("actor", actor)

        self.GC.Start()

    def step(self, k, mi):
        c = self.stats

        c.reset_on_new_model()

        self.trainer.setup(sampler=self.sampler, mi=mi, rl_method=None)
        self.trainer.episode_start(k)

        if self.tqdm:
            import tqdm
            tq = tqdm.tqdm(total=self.args.num_eval)
            while c.count_completed() < self.args.num_eval:
                old_n = c.count_completed()
                self.GC.Run()
                diff = c.count_completed() - old_n
                tq.update(diff)
            tq.close()
        else:
            while c.count_completed() < self.args.num_eval:
                self.GC.Run()

        c.print_summary()

    def __del__(self):
        self.GC.Stop()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    use_multi_process = int(os.environ.get("multi_process", 0))

    sampler = Sampler()
    trainer = Trainer()
    game = load_module(os.environ["game"]).Loader()
    runner = MultiProcessRun() if use_multi_process else SingleProcessRun()
    model_file = load_module(os.environ["model_file"])
    model_class, method_class = model_file.Models[os.environ["model"]]

    model_loader = ModelLoader(model_class)
    method = method_class()

    args_providers = [sampler, trainer, game, runner, model_loader, method]

    eval_only = os.environ.get("eval_only", False)
    has_eval_process = os.environ.get("eval_process", False)
    if has_eval_process or eval_only:
        eval_process = EvaluationProcess()
        evaluator = Eval()

        args_providers.append(eval_process)
        args_providers.append(evaluator)
    else:
        eval_process = None

    all_args = ArgsProvider.Load(parser, args_providers)

    GC = game.initialize()
    GC.setup_gpu(all_args.gpu)
    all_args.method_class = method_class

    model = model_loader.load_model(GC.params)
    mi = ModelInterface()
    mi.add_model("model", model, optim_params={ "lr" : 0.001})
    mi.add_model("actor", model, copy=True, cuda=True, gpu_id=all_args.gpu)
    method.set_model_interface(mi)

    trainer.setup(sampler=sampler, mi=mi, rl_method=method)

    if use_multi_process:
        GC.reg_callback("actor", trainer.actor)
        runner.setup(GC, mi, remote_init, remote_run,
                     episode_start=trainer.episode_start,
                     episode_summary=trainer.episode_summary, args=all_args)
        runner.run()
    else:
        if eval_only:
            eval_process.set(evaluator, all_args)
            eval_process.run_same_process(mi.clone(all_args.eval_gpu))
        else:
            def train_and_update(sel, sel_gpu):
                reply = trainer.train(sel, sel_gpu)
                model_updated = trainer.just_updated
                if model_updated and eval_process is not None:
                    eval_process.update_model("actor", mi["actor"])
                return reply

            GC.reg_callback("train", train_and_update)
            GC.reg_callback("actor", trainer.actor)
            runner.setup(GC, episode_summary=trainer.episode_summary,
                        episode_start=trainer.episode_start)

            if has_eval_process:
                eval_process.set(evaluator, all_args)
                eval_process.start()
                eval_process.set_model(mi)

            runner.run()

