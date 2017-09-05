# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import torch
import torch.nn as nn
from collections import Counter

from rlpytorch import Model, ActorCritic
from trunk import MiniRTSNet

class Model_ActorCritic(Model):
    def __init__(self, args):
        super(Model_ActorCritic, self).__init__(args)
        self._init(args)

    def _init(self, args):
        params = args.params
        assert isinstance(params["num_action"], int), "num_action has to be a number. action = " + str(params["num_action"])
        self.params = params
        self.net = MiniRTSNet(args)

        if self.params.get("model_no_spatial", False):
            self.num_unit = params["num_unit_type"]
            linear_in_dim = (params["num_unit_type"] + 7)
        else:
            linear_in_dim = (params["num_unit_type"] + 7) * 25

        self.linear_policy = nn.Linear(linear_in_dim, params["num_action"])
        self.linear_value = nn.Linear(linear_in_dim, 1)
        self.softmax = nn.Softmax()

    def get_define_args():
        return MiniRTSNet.get_define_args()

    def forward(self, x):
        if self.params.get("model_no_spatial", False):
            # Replace a complicated network with a simple retraction.
            # Input: batchsize, channel, height, width
            xreduced = x["s"].sum(2).sum(3).squeeze()
            xreduced[:, self.num_unit:] /= 20 * 20
            output = self._var(xreduced)
        else:
            s, res = x["s"], x["res"]
            output = self.net(self._var(s), self._var(res))

        policy = self.softmax(self.linear_policy(output))
        value = self.linear_value(output)
        return dict(V=value, pi=policy)

# Format: key, [model, method]
# if method is None, fall back to default mapping from key to method
Models = {
    "actor_critic": [Model_ActorCritic, ActorCritic],
}
