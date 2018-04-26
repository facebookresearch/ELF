# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import torch
import torch.nn as nn
from torch.autograd import Variable
import math

from ..args_provider import ArgsProvider
from .discounted_reward import DiscountedReward
from .value_matcher import ValueMatcher
from .utils import add_err

# Q learning
class Q_learning:
    ''' An actor critic model '''
    def __init__(self):
        ''' Initialization of `DiscountedReward` and `ValueMatcher`.
        Initialize the arguments needed (num_games, batchsize, value_node) and in child_providers.
        '''
        self.discounted_reward = DiscountedReward()
        self.q_loss = nn.SmoothL1Loss().cuda()

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("a_node", "a"),
                ("Q_node", "Q")
            ],
            more_args = ["num_games", "batchsize"],
            child_providers = [ self.discounted_reward.args ],
        )

    def update(self, mi, batch, stats):
        ''' Actor critic model update.
        Feed stats for later summarization.

        Args:
            mi(`ModelInterface`): mode interface used
            batch(dict): batch of data. Keys in a batch:
                ``s``: state,
                ``r``: immediate reward,
                ``terminal``: if game is terminated
            stats(`Stats`): Feed stats for later summarization.
        '''
        m = mi["model"]
        args = self.args
        Q_node = args.Q_node
        a_node = args.a_node

        T = batch["s"].size(0)

        state_curr = m(batch.hist(T - 1))
        Q = state_curr[Q_node].squeeze().data
        V = Q.max(1)
        self.discounted_reward.setR(V, stats)

        err = None

        for t in range(T - 2, -1, -1):
            bht = batch.hist(t)
            state_curr = m.forward(bht)

            # go through the sample and get the rewards.
            Q = state_curr[Q_node].squeeze()
            a = state_curr[a_node].squeeze()

            R = self.discounted_reward.feed(
                dict(r=batch["r"][t], terminal=batch["terminal"][t]),
                stats=stats)

            # Then you want to match Q value here.
            # Q: batchsize * #action.
            Q_sel = Q.gather(1, a.view(-1, 1)).squeeze()
            err = add_err(err, nn.L2Loss(Q_sel, Variable(R)))

        stats["cost"].feed(err.data[0] / (T - 1))
        err.backward()
