# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
import torch
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'elf'))
import utils_elf
from ..args_provider import ArgsProvider
from ..stats import Stats
from .timer import RLTimer
from .utils import ModelSaver, MultiCounter
from datetime import datetime

# import torch.multiprocessing as _mp
# mp = _mp.get_context('spawn')

class Evaluator:
    def __init__(self, name="eval", stats=True, verbose=False, actor_name="actor"):
        ''' Initialization for Evaluator. Accepted arguments: ``num_games``, ``batch_size``, ``num_minibatch``
        '''
        if stats:
            self.stats = Stats(name)
            child_providers = [ self.stats.args ]
        else:
            self.stats = None
            child_providers = []

        self.name = name
        self.actor_name = actor_name
        self.verbose = verbose
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("keys_in_reply", "")
            ],
            more_args = ["num_games", "batchsize", "num_minibatch"],
            on_get_args = self._on_get_args,
            child_providers = child_providers
        )

    def _on_get_args(self, _):
        if self.stats is not None and not self.stats.is_valid():
            self.stats = None
        self.keys_in_reply = set(self.args.keys_in_reply.split(","))

    def episode_start(self, i):
        ''' Called before each episode. Reset ``actor_count`` to 0.

        Args:
            i(int): index in the minibatch
        '''
        self.actor_count = 0

    def actor(self, batch):
        ''' Actor.
        Get the model, forward the batch and get a distribution. Sample from it and act.
        Reply the message to game engine.

        Args:
            batch(dict): batch data

        Returns:
            reply_msg(dict): ``pi``: policy, ``a``: action, ``V``: value, `rv`: reply version, signatured by step
        '''
        if self.verbose: print("In Evaluator[%s]::actor" % self.name)

        # actor model.
        m = self.mi[self.actor_name]
        with torch.no_grad():
            state_curr = m.forward(batch.hist(0))

        if self.sampler is not None:
            reply_msg = self.sampler.sample(state_curr)
        else:
            reply_msg = dict(pi=state_curr["pi"].data)

        if self.stats is not None:
            self.stats.feed_batch(batch)

        if "rv" in self.keys_in_reply:
            reply_msg["rv"] = self.mi["actor"].step

        if "V" in self.keys_in_reply:
            reply_msg["V"] = state_curr["V"].data

        self.actor_count += 1
        return reply_msg

    def episode_summary(self, i):
        ''' Called after each episode. Print stats and summary

        Args:
            i(int): index in the minibatch
        '''
        print("[%s] actor count: %d/%d" % (self.name, self.actor_count, self.args.num_minibatch))

        if self.stats is not None:
            self.stats.print_summary()
            if self.stats.count_completed() > 10000:
                self.stats.reset()

    def setup(self, mi=None, sampler=None):
        ''' Setup `ModelInterface` and `Sampler`. Resetting stats.

        Args:
            mi(`ModelInterface`)
            sample(`Sampler`)
        '''
        self.mi = mi
        self.sampler = sampler

        if self.stats is not None:
            self.stats.reset()


class Trainer:
    def __init__(self, verbose=False, actor_name="actor"):
        ''' Initialization for Trainer. Accepted arguments: ``num_games``, ``batch_size``
            Also need arguments for `Evaluator` and `ModelSaver` class.
        '''
        self.timer = RLTimer()
        self.verbose = verbose
        self.last_time = None
        self.evaluator = Evaluator("trainer", verbose=verbose, actor_name=actor_name)
        self.saver = ModelSaver()
        self.counter = MultiCounter(verbose=verbose)

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("freq_update", 1),
            ],
            more_args = ["num_games", "batchsize"],
            child_providers = [ self.evaluator.args, self.saver.args ],
        )
        self.just_update = False

    def actor(self, batch):
        ''' Actor.
        Get the model, forward the batch and get a distribution. Sample from it and act.
        Reply the message to game engine.

        Args:
            batch(dict): batch data

        Returns:
            reply_msg(dict): ``pi``: policy, ``a``: action, ``V``: value, `rv`: reply version, signatured by step
        '''
        self.counter.inc("actor")
        return self.evaluator.actor(batch)

    def train(self, batch):
        ''' Trainer.
        Get the model, forward the batch and update the weights.

        Args:
            batch(dict): batch data
        '''
        mi = self.evaluator.mi

        self.counter.inc("train")
        self.timer.Record("batch_train")

        mi.zero_grad()
        self.rl_method.update(mi, batch, self.counter.stats)
        mi.update_weights()

        self.timer.Record("compute_train")
        if self.counter.counts["train"] % self.args.freq_update == 0:
            # Update actor model
            # print("Update actor model")
            # Save the current model.
            mi.update_model("actor", mi["model"])
            self.just_updated = True

        self.just_updated = False

    def episode_start(self, i):
        ''' Called before each episode.

        Args:
            i(int): index in the minibatch
        '''
        self.evaluator.episode_start(i)

    def episode_summary(self, i):
        ''' Called after each episode. Print stats and summary. Also print arguments passed in.

        Args:
            i(int): index in the minibatch
        '''
        args = self.args

        prefix = "[%s][%d] Iter" % (str(datetime.now()), args.batchsize) + "[%d]: " % i
        print(prefix)
        if self.counter.counts["train"] > 0:
            self.saver.feed(self.evaluator.mi["model"])

        print("Command arguments " + str(args.command_line))
        self.counter.summary(global_counter=i)
        print("")

        self.evaluator.episode_summary(i)
        self.timer.Restart()

    def setup(self, rl_method=None, mi=None, sampler=None):
        ''' Setup `RLMethod`, ModelInterface` and `Sampler`

        Args:
            rl_method(RLmethod)
            mi(`ModelInterface`)
            sample(`Sampler`)
        '''
        self.rl_method = rl_method
        self.evaluator.setup(mi=mi, sampler=sampler)
