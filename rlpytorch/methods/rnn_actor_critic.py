# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import torch
import torch.nn as nn
from torch.autograd import Variable
import math

from ..args_provider import ArgsProvider

from .utils import add_err
from .policy_gradient import PolicyGradient
from .discounted_reward import DiscountedReward
from .value_matcher import ValueMatcher

# Actor critic model.
class RNNActorCritic:
    def __init__(self):
        self.pg = PolicyGradient()
        self.discounted_reward = DiscountedReward()
        self.value_matcher = ValueMatcher()

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
            ],
            more_args = ["num_games", "batchsize", "value_node"],
            child_providers = [ self.pg.args, self.discounted_reward.args, self.value_matcher.args ],
        )

    def update(self, mi, batch, hiddens, stats):
        ''' Actor critic model '''
        m = mi["model"]
        args = self.args
        value_node = self.args.value_node

        T = batch["a"].size(0)

        h = Variable(hiddens)
        hs = [ ]
        ss = [ ]

        # Forward to compute LSTM.
        for t in range(0, T - 1):
            if t > 0:
                term = Variable(1.0 - batch["terminal"][t].float()).view(-1, 1)
                h.register_hook(lambda grad: grad.mul(term))

            state_curr = m(batch.hist(t), h)
            h = m.transition(state_curr["h"], batch["a"][t])
            hs.append(h)
            ss.append(state_curr)

        R = ss[-1][value_node].squeeze().data
        self.discounted_reward.setR(R, stats)

        err = None

        # Backward to compute gradient descent.
        for t in range(T - 2, -1, -1):
            state_curr = ss[t]

            # go through the sample and get the rewards.
            bht = batch.hist(t)
            V = state_curr[value_node].squeeze()

            R = self.discounted_reward.feed(
                dict(r=batch["r"][t], terminal=batch["terminal"][t]),
                stats)

            err = add_err(err, self.pg.feed(R - V.data, state_curr, bht, stats, old_pi_s=bht))
            err = add_err(err, self.value_matcher.feed({ value_node : V, "target" : R }, stats))

        stats["cost"].feed(err.data[0] / (T-1))
        err.backward()

