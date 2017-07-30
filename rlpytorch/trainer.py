# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

from .rltimer import RLTimer
from .rlsampler import sample_multinomial, epsilon_greedy

import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'elf'))
import utils_elf
from .args_utils import ArgsProvider
from .parameter_server import SharedData, ParameterServer

import threading
import tqdm
import pickle

from collections import defaultdict
from datetime import datetime

import torch.multiprocessing as _mp
mp = _mp.get_context('spawn')

class Sampler:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("sample_policy", dict(type=str, choices=["epsilon-greedy", "multinomial", "uniform"], help="Sample policy", default="epsilon-greedy")),
                ("sample_node", dict(type=str, default="pi")),
                ("greedy", dict(action="store_true")),
                ("epsilon", dict(type=float, help="Used in epsilon-greedy approach", default=0.00))
            ]
        )

    def sample(self, state_curr):
        if self.args.greedy:
            # print("Use greedy approach")
            action = epsilon_greedy(state_curr, self.args, node="pi")
        else:
            # print("Use multinomial approach")
            action = sample_multinomial(state_curr, self.args, node="pi")
        return action


class Trainer:
    def __init__(self):
        self.timer = RLTimer()
        self.last_time = None
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("freq_update", 1),
                ("record_dir", "./record"),
                ("save_prefix", "save"),
                ("save_dir", dict(type=str, default=None)),
            ],
            more_args = ["num_games", "batchsize", "num_minibatch"],
            on_get_args = self._on_get_args
        )
        self.just_update = False

    def _on_get_args(self, _):
        args = self.args
        args.save = (args.num_games == args.batchsize)
        if args.save and not os.path.exists(args.record_dir):
            os.mkdir(args.record_dir)

        # Use environment variable "save" if there is any.
        if args.save_dir is None:
            args.save_dir = os.environ.get("save", "./")

    def train(self, sel, sel_gpu):
        # training procedure.
        self.timer.Record("batch_train")
        self.rl_method.run(sel_gpu)
        self.timer.Record("compute_train")
        if self.args.save:
            pickle.dump(
                utils_elf.to_numpy(sel),
                open(os.path.join(self.args.record_dir, "record-train-%d.bin" % self.train_count), "wb"),
                protocol=2
            )
        self.train_count += 1
        if self.train_count % self.args.freq_update == 0:
            # Update actor model
            # print("Update actor model")
            # Save the current model.
            self.mi.update_model("actor", self.mi["model"])
            self.just_updated = True

        self.just_updated = False

    def actor(self, sel, sel_gpu):
        # actor model.
        self.timer.Record("batch_actor")
        state_curr = self.mi.forward("actor", sel_gpu.hist(0))
        action = self.sampler.sample(state_curr)

        reply_msg = dict(pi=state_curr["pi"].data, a=action, V=state_curr["V"].data, rv=self.mi["actor"].step)

        self.timer.Record("compute_actor")
        self.actor_count += 1

        return reply_msg

    def episode_start(self, i):
        self.train_count = 0
        self.actor_count = 0

    def episode_summary(self, i):
        args = self.args

        this_time = datetime.now()
        if self.last_time is not None:
            print("[%d] Time spent = %f ms" % (i, (this_time - self.last_time).total_seconds() * 1000))
        self.last_time = this_time

        prefix = "[%s][%d] Iter" % (str(datetime.now()), args.batchsize) + "[%d]: " % i
        print(prefix)
        print("Train count: %d/%d, actor count: %d/%d" % (self.train_count, args.num_minibatch, self.actor_count, args.num_minibatch))
        print("Save to " + args.save_dir)
        if self.train_count > 0:
            filename = os.path.join(args.save_dir, args.save_prefix + "-%d.bin" % self.mi["model"].step)
            print("Filename = " + filename)
            self.mi["model"].save(filename)

        print("Command arguments " + str(args.command_line))
        self.rl_method.print_stats(global_counter=i)
        print("")

        self.timer.Restart()

    def setup(self, rl_method=None, mi=None, sampler=None):
        self.rl_method = rl_method
        self.mi = mi
        self.sampler = sampler


class SingleProcessRun:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("num_minibatch", 5000),
                ("num_episode", 10000),
                ("tqdm", dict(action="store_true")),
            ]
        )

    def setup(self, GC, episode_start=None, episode_summary=None):
        self.GC = GC
        self.episode_summary = episode_summary
        self.episode_start = episode_start

    def run(self):
        self.GC.Start()
        args = self.args
        for k in range(args.num_episode):
            if self.episode_start is not None:
                self.episode_start(k)
            if args.tqdm: iterator = tqdm.trange(args.num_minibatch)
            else: iterator = range(args.num_minibatch)

            for i in iterator:
                self.GC.Run()

            if self.episode_summary is not None:
                self.episode_summary(k)

        self.GC.PrintSummary()
        self.GC.Stop()

    def run_multithread(self):
        def train_thread():
            args = self.args
            for i in range(args.num_episode):
                for k in range(args.num_minibatch):
                    if self.episode_start is not None:
                        self.episode_start(k)

                    if k % 500 == 0:
                        print("Receive minibatch %d/%d" % (k, args.num_minibatch))

                    self.GC.RunGroup("train")

                # Print something.
                self.episode_summary(i)

        def actor_thread():
            while True:
                self.GC.RunGroup("actor")

        self.GC.Start()

        # Start the two threads.
        train_th = threading.Thread(target=train_thread)
        actor_th = threading.Thread(target=actor_thread)
        train_th.start()
        actor_th.start()
        train_th.join()
        actor_th.join()

class EvaluationProcess(mp.Process):
    def __init__(self):
        super(EvaluationProcess, self).__init__()
        self.server = ParameterServer(2)
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("eval_freq", 10),
                ("eval_gpu", 1),
            ]
        )
        self.count = 0

    def set_model(self, mi):
        self.server.server_send_model(mi)

    def update_model(self, key, mi, immediate=False):
        if (self.count % self.args.eval_freq == 0) or immediate:
            self.server.server_update_model(key, mi, noblock=True)

        self.count += 1

    def set(self, evaluator, args):
        self.evaluator = evaluator
        self.args = args

    def run(self):
        ''' Run the model '''
        self.server.client_receive_model()
        self.evaluator.setup(self.args)
        k = 0
        while True:
            mi = self.server.client_refresh_model(gpu=self.evaluator.gpu)
            print("Eval: Get refreshed model")

            # Do your evaluation.
            self.evaluator.step(k, mi)
            k += 1

    def run_same_process(self, mi):
        self.evaluator.setup(self.args)
        # Do your evaluation.
        self.evaluator.step(0, mi)


class MultiProcessRun:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("num_minibatch", 5000),
                ("num_episode", 10000),
                ("num_process", 2),
                ("tqdm", dict(action="store_true")),
            ]
        )

    def setup(self, GC, mi, remote_init, remote_process,
              episode_start=None, episode_summary=None, args=None):
        self.GC = GC
        self.episode_start = episode_start
        self.episode_summary = episode_summary
        self.remote_init = remote_init
        self.remote_process = remote_process
        self.shared_data = \
            SharedData(self.args.num_process, mi, GC.inputs[1][0],
                       cb_remote_initialize=remote_init,
                       cb_remote_batch_process=remote_process, args=args)

        self.total_train_count = 0
        self.success_train_count = 0

    def _train(self, sel, sel_gpu):
        # Send to remote for remote processing.
        self.total_train_count += 1
        success = self.shared_data.send_batch(sel)
        if success: self.success_train_count += 1

    def run(self):
        self.GC.reg_callback("train", self._train)

        self.GC.Start()
        args = self.args

        for k in range(args.num_episode):
            if self.episode_start is not None:
                self.episode_start(k)
            if args.tqdm: iterator = tqdm.trange(args.num_minibatch)
            else: iterator = range(args.num_minibatch)

            for i in iterator:
                self.GC.Run()

            if self.episode_summary is not None:
                self.episode_summary(k)

            print("Train stat: (%.2f) %d/%d" % (float(self.success_train_count) / self.total_train_count, self.success_train_count, self.total_train_count))

        self.GC.PrintSummary()
        self.GC.Stop()

