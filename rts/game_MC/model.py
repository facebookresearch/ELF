# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import torch
import torch.nn as nn

from rlpytorch import Model, ActorCritic

class MiniRTSNet(Model):
    def __init__(self, args):
        # this is the place where you instantiate all your modules
        # you can later access them using the same names you've given them in here
        super(MiniRTSNet, self).__init__(args)
        self.m = args.params["num_unit_type"] + 7
        self.conv1 = nn.Conv2d(self.m, self.m, 3, padding = 1)
        self.pool1 = nn.MaxPool2d(2, 2)
        self.conv2 = nn.Conv2d(self.m, self.m, 3, padding = 1)

        self.pool2 = nn.MaxPool2d(2, 2)
        self.conv3 = nn.Conv2d(self.m, self.m, 3, padding = 1)
        self.conv4 = nn.Conv2d(self.m, self.m, 3, padding = 1)
        self.relu = nn.ReLU() if self._no_leaky_relu() else nn.LeakyReLU(0.1)

        if not self._no_bn():
            self.conv1_bn = nn.BatchNorm2d(self.m)
            self.conv2_bn = nn.BatchNorm2d(self.m)
            self.conv3_bn = nn.BatchNorm2d(self.m)
            self.conv4_bn = nn.BatchNorm2d(self.m)

    def _no_bn(self):
        return getattr(self.args, "disable_bn", False)

    def _no_leaky_relu(self):
        return getattr(self.args, "disable_leaky_relu", False)

    def forward(self, input, res):
        # BN and LeakyReLU are from Wendy's code.
        h1 = self.conv1(input.view(input.size(0), self.m, 20, 20))
        if not self._no_bn(): h1 = self.conv1_bn(h1)
        h1 = self.relu(h1)

        h2 = self.conv2(self.relu(h1))
        if not self._no_bn(): h2 = self.conv2_bn(h2)
        h2 = self.relu(h2)

        h2 = self.pool1(h2)

        h3 = self.conv3(self.relu(h2))
        if not self._no_bn(): h3 = self.conv3_bn(h3)
        h3 = self.relu(h3)

        h4 = self.conv4(self.relu(h3))
        if not self._no_bn(): h4 = self.conv4_bn(h4)
        h4 = self.relu(h4)

        h4 = self.pool2(h4)
        x = h4.view(h4.size(0), -1)
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
