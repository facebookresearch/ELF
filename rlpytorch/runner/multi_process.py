# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

from ..args_provider import ArgsProvider
from .parameter_server import SharedData, ParameterServer
import tqdm

class MultiProcessRun:
    def __init__(self):
        ''' Initialization for MultiProcessRun.
        Accepted arguments:
        ``num_minibatch``,
        ``num_episode``,
        ``num_process``,
        ``tqdm``
        '''
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
        ''' Setup for MultiProcessRun.

        Args:
            GC(`GameContext`): Game Context
            mi(`ModelInterface`): ModelInterface
            remote_init(func): Callbacks for remote initialization, used in SharedData
            remote_process(func): Callbacks for remote process, used in SharedData
            episode_start(func): operations to perform before each episode
            episode_summary(func): operations to summarize after each epidsode
            args(dict): Additional arguments for class `SharedData`
        '''
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

    def _train(self, batch):
        # Send to remote for remote processing.
        # TODO Might have issues when batch is on GPU.
        self.total_train_count += 1
        success = self.shared_data.send_batch(batch)
        if success: self.success_train_count += 1

    def run(self):
        ''' Main training loop. Initialize Game Context and looping the required episodes.
            Call episode_start and episode_summary before and after each episode if necessary.
            Visualize with a progress bar if ``tqdm`` is set.
            Print training stats after each episode.
            In the end, print summary for game context and stop it.
        '''
        self.GC.reg_callback("train", self._train)

        self.GC.Start()
        args = self.args

        for k in range(args.num_episode):
            if self.episode_start is not None:
                self.episode_start(k)
            if args.tqdm: iterator = tqdm.trange(args.num_minibatch, ncols=50)
            else: iterator = range(args.num_minibatch)

            for _ in iterator:
                self.GC.Run()

            if self.episode_summary is not None:
                self.episode_summary(k)

            print("Train stat: (%.2f) %d/%d" % (float(self.success_train_count) / self.total_train_count, self.success_train_count, self.total_train_count))

        self.GC.PrintSummary()
        self.GC.Stop()
