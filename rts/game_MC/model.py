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

class MiniRTSNet(Model):
    def __init__(self, args):
        # this is the place where you instantiate all your modules
        # you can later access them using the same names you've given them in here
        super(MiniRTSNet, self).__init__(args)
        self.m = args.params["num_unit_type"] + 7
        self.pool = nn.MaxPool2d(2, 2)

        self.num_channels = [ self.m, 64, 64, 128, 128, 128, 64, 64, self.m ]

        self.convs = [ nn.Conv2d(self.num_channels[i], self.num_channels[i+1], 3, padding = 1) for i in range(len(self.num_channels)-1) ]
        for i, conv in enumerate(self.convs):
            setattr(self, "conv%d" % i, conv)

        self.relu = nn.ReLU() if self._no_leaky_relu() else nn.LeakyReLU(0.1)

        if not self._no_bn():
            self.convs_bn = [ nn.BatchNorm2d(conv.out_channels) for conv in self.convs ]
            for i, conv_bn in enumerate(self.convs_bn):
                setattr(self, "conv%d_bn" % i, conv_bn)

        # self.arch = "ccpccp"
        self.arch = "ccccpccccp"

    def _no_bn(self):
        return getattr(self.args, "disable_bn", False)

    def _no_leaky_relu(self):
        return getattr(self.args, "disable_leaky_relu", False)

    def forward(self, input, res):
        # BN and LeakyReLU are from Wendy's code.
        x = input.view(input.size(0), self.m, 20, 20)

        counts = Counter()
        for i in range(len(self.arch)):
            if self.arch[i] == "c":
                c = counts["c"]
                x = self.convs[c](x)
                if not self._no_bn(): x = self.convs_bn[c](x)
                x = self.relu(x)
                counts["c"] += 1
            elif self.arch[i] == "p":
                x = self.pool(x)

        x = x.view(x.size(0), -1)
        return x

class Model_ActorCritic(Model):
    def __init__(self, args):
        super(Model_ActorCritic, self).__init__(args)

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
        return value, dict(V=value, pi=policy)

# Format: key, [model, method]
# if method is None, fall back to default mapping from key to method
Models = {
    "actor_critic": [Model_ActorCritic, ActorCritic],
}
