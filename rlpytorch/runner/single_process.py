# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

from ..args_provider import ArgsProvider
import tqdm

class SingleProcessRun:
    def __init__(self):
        ''' Initialization for SingleProcessRun. Accepted arguments:
        ``num_minibatch``,

        ``num_episode``,

        ``tqdm``
        '''
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("num_minibatch", 5000),
                ("num_episode", 10000),
                ("tqdm", dict(action="store_true")),
            ]
        )

    def setup(self, GC, episode_start=None, episode_summary=None):
        ''' Setup for SingleProcessRun.

        Args:
            GC(`GameContext`): Game Context
            episode_start(func): operations to perform before each episode
            episode_summary(func): operations to summarize after each epidsode
        '''
        self.GC = GC
        self.episode_summary = episode_summary
        self.episode_start = episode_start

    def run(self):
        ''' Main training loop. Initialize Game Context and looping the required episodes.
            Call episode_start and episode_summary before and after each episode if necessary.
            Visualize with a progress bar if ``tqdm`` is set.
            Print training stats after each episode.
            In the end, print summary for game context and stop it.
        '''
        # import pdb
        # pdb.set_trace()
        self.GC.Start()
        args = self.args
        for k in range(args.num_episode):
            if self.episode_start is not None:
                self.episode_start(k)
            if args.tqdm: iterator = tqdm.trange(args.num_minibatch, ncols=50)
            else: iterator = range(args.num_minibatch)

            for i in iterator:
                self.GC.Run()
                

            if self.episode_summary is not None:
                self.episode_summary(k)

        self.GC.PrintSummary()
        self.GC.Stop()

    def run_multithread(self):
        ''' Start training in a multithreaded environment '''
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
