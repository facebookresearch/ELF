# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

from .rlmethod_base import *
import torch
import torch.nn as nn
from torch.autograd import Variable
import math

from .args_utils import ArgsProvider
from .policy_gradient import PolicyGradient
from .discounted_reward import DiscountedReward

# Actor critic model.
class ActorCritic:
    def __init__(self, args=None):
        self.pg = PolicyGradient()
        self.discounted_reward = DiscountedReward()

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
            ],
            more_args = ["num_games", "batchsize"],
            child_providers = [ self.pg.args, self.discounted_reward.args ],
            fixed_args = args
        )

    def update(self, mi, batch, stats):
        ''' Actor critic model '''
        m = mi["model"]
        args = self.args

        T = batch["a"].size(0)

        state_curr = m(batch.hist(T - 1))
        self.discounted_reward.setR(state_curr["V"].squeeze().data)

        for t in range(T - 2, -1, -1):
            state_curr = m(batch.hist(t))

            # go through the sample and get the rewards.
            a = batch["a"][t]
            V = state_curr["V"].squeeze()

            R = self.discounted_reward.feed(
                dict(r=batch["r"][t], terminal=batch["terminal"][t]),
                stats=stats)

            overall_err = self.pg.feed(
                dict(
                    old_pi = Variable(batch["pi"][t]),
                    V = V,
                    R = Variable(R),
                    a = Variable(a),
                    pi = state_curr["pi"]
                ),
                stats=stats)

            overall_err.backward()

            stats["cost"].feed(overall_err.data[0])
            stats["predict_reward"].feed(V.data[0])
            #print("[%d]: reward=%.4f, sum_reward=%.2f, acc_reward=%.4f, value_err=%.4f, policy_err=%.4f" % (i, r.mean(), r.sum(), R.mean(), value_err.data[0], policy_err.data[0]))

