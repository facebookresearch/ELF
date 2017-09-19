import torch
import torch.nn as nn
import math
from ..args_provider import ArgsProvider

class DiscountedReward:
    def __init__(self):
        ''' Initialization discounted_reward.
        Accepted arguments:
        ``discount``: discount factor of reward.'''
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("discount", dict(type=float, default=0.99)),
            ],
        )

    def setR(self, R, stats):
        ''' Set rewards and feed to stats'''
        self.R = R
        stats["init_reward"].feed(R.mean())

    def feed(self, batch, stats):
        '''
        Update discounted reward and feed to stats.

        Keys in a batch:

        ``r`` (tensor): immediate reward.

        ``terminal`` (tensor): whether the current game has terminated.

        Feed to stats: immediate reward and accumulated reward
        '''
        r = batch["r"]
        term = batch["terminal"]

        # Compute the reward.
        self.R = self.R * self.args.discount + r

        # If we see any terminal signal, break the reward backpropagation chain.
        for i, terminal in enumerate(term):
            if terminal:
                self.R[i] = r[i]

        stats["reward"].feed(r.mean())
        stats["acc_reward"].feed(self.R.mean())

        return self.R
