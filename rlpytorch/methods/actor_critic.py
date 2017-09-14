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
from .policy_gradient import PolicyGradient
from .discounted_reward import DiscountedReward
from .value_matcher import ValueMatcher
from .utils import add_err

# Actor critic model.
class ActorCritic:
    def __init__(self):
        self.pg = PolicyGradient()
        self.discounted_reward = DiscountedReward()
        self.value_matcher = ValueMatcher()

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
            ],
            more_args = ["num_games", "batchsize"],
            child_providers = [ self.pg.args, self.discounted_reward.args, self.value_matcher.args ],
        )

    def update(self, mi, batch, stats):
        ''' Actor critic model '''
        m = mi["model"]
        args = self.args

        T = batch["a"].size(0)

        state_curr = m(batch.hist(T - 1))
        self.discounted_reward.setR(state_curr["V"].squeeze().data, stats)

        err = None

        for t in range(T - 2, -1, -1):
            state_curr = m(batch.hist(t))

            # go through the sample and get the rewards.
            a = batch["a"][t]
            V = state_curr["V"].squeeze()

            R = self.discounted_reward.feed(
                dict(r=batch["r"][t], terminal=batch["terminal"][t]),
                stats=stats)

            err = add_err(err, self.pg.feed(dict(old_pi=batch["pi"][t], Q=R - V.data, a=a, pi=state_curr["pi"]), stats))
            err = add_err(err, self.value_matcher.feed(dict(V=V, target=R), stats))

        stats["cost"].feed(err.data[0] / (T - 1))
        err.backward()


