# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import torch
import torch.nn as nn

from model_base import *
from rlmethod_forward import ForwardPrediction, ForwardPredictionSample
from rlmethod_common import ActorCritic, Q_learning

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

    def forward(self, input, r0, r1):
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
        linear_in_dim = (params["num_unit_type"] + 7) * 25
        self.linear_policy = nn.Linear(linear_in_dim, params["num_action"])
        self.linear_value = nn.Linear(linear_in_dim, 1)
        self.softmax = nn.Softmax()

    def forward(self, x):
        s, r0, r1 = x["s"], x["r0"], x["r1"]
        output = self.net(self._var(s), self._var(r0), self._var(r1))
        policy = self.softmax(self.linear_policy(output))
        value = self.linear_value(output)
        return value, dict(V=value, pi=policy)

    def test(self):
        x = torch.cuda.FloatTensor(max_s, max_s)
        x.fill_(0)
        for i in range(max_s):
            x[i, i] = 1

        _, res = self(self._var(x))
        # Check both policy and value function
        print(res["pi"].exp())
        print(res["V"])


class Model_Q(Model):
    def __init__(self, args):
        super(Model_Q, self).__init__(args)

        params = args.params
        assert isinstance(params["num_action"], int), "num_action has to be a number. action = " + str(params["num_action"])
        self.params = params
        self.net = MiniRTSNet(args)
        linear_in_dim = (params["num_unit_type"] + 7) * 25
        self.linear_Q = nn.Linear(linear_in_dim, params["num_action"])

    def forward(self, x):
        s, r0, r1 = x["s"], x["r0"], x["r1"]
        output = self.net(self._var(s), self._var(r0), self._var(r1))
        Q = self.linear_Q(output)
        return output, dict(Q=Q)


class Model_ActorCritic_ActionMap(Model):
    def __init__(self, args):
        super(Model_ActorCritic_ActionMap, self).__init__(args)

        params = args.params

        assert isinstance(params["num_action"], int), "num_action has to be a number. action = " + str(params["num_action"])
        self.params = params
        self.net = MiniRTSNet(args)
        linear_in_dim = (params["num_unit_type"] + 7) * 25
        self.linear_value = nn.Linear(linear_in_dim, 1)

        self.m = params["num_unit_type"] + 7
        self.rx = getattr(args, "actionmap_x", 2)
        self.ry = getattr(args, "actionmap_y", 2)
        self.rc = params["region_range_channel"]

        assert self.rx <= params["region_max_range_x"], "rx [%d] has to be smaller than max_range [%d]" % (self.rx, params["region_max_range_x"])
        assert self.ry <= params["region_max_range_y"], "ry [%d] has to be smaller than max_range [%d]" % (self.ry, params["region_max_range_y"])
        # print("ActionMap: (rx, ry) = (%d, %d)" % (self.rx, self.ry))

        # Generate rx-by-ry outputs.
        self.linear_policies = []
        for i in range(self.rx):
            curr_linear_layers = []
            for j in range(self.ry):
                curr_linear_layer = nn.Linear(linear_in_dim, params["num_action"])
                # This is used so that .cuda() are aware of these layers.
                setattr(self, "linear_policy_%d_%d" % (i, j), curr_linear_layer)
                curr_linear_layers.append(curr_linear_layer)
            self.linear_policies.append(curr_linear_layers)

        # Softmax along region_range_x * region_range_y
        self.softmax = nn.Softmax()

    def forward(self, x):
        s, r0, r1 = x["s"], x["r0"], x["r1"]
        output = self.net(self._var(s), self._var(r0), self._var(r1))
        value = self.linear_value(output)

        # result would be a list of list of self._vars.
        policies = [ [ self.softmax(self.linear_policies[i][j](output)) for j in range(self.ry) ] for i in range(self.rx) ]
        return value, dict(V=value, pi=policies)

class Model_Forward(Model):
    def __init__(self, args):
        super(Model_Forward, self).__init__(args)
        params = args.params
        self.params = params
        self.net = MiniRTSNet(args)
        linear_in_dim = (params["num_unit_type"] + 7) * 25
        self.T = 6
        self.outputs = ["num_unit", "total_hp_ratio", "r0"]

        self.linear = nn.Linear(linear_in_dim, len(self.outputs) * self.T)

    def forward(self, x):
        s, r0, r1 = x["s"], x["r0"], x["r1"]
        output = self.net(self._var(s), self._var(r0), self._var(r1))
        pred = self.linear(output)
        preds = { "fa_%s_T%d" % (k, t) : pred[:, t * len(self.outputs) + i] for i, k in enumerate(self.outputs) for t in range(self.T) }
        return pred, preds


class Model_Forward_Sample(Model):
    def __init__(self, args):
        super(Model_Forward_Sample, self).__init__(args)
        params = args.params
        self.params = params
        self.net = MiniRTSNet(args)
        linear_in_dim = (params["num_unit_type"] + 7) * 25
        self.T = 6
        self.outputs = ["fa_myr0", "fa_myworker", "fa_mytroop", "fa_mybarrack"]
        self.lengths = [5, 4, 6, 2]
        #resource
        self.linear1 = nn.Linear(linear_in_dim, 5 * self.T)
        #worker
        self.linear2 = nn.Linear(linear_in_dim, 4 * self.T)
        #troop
        self.linear3 = nn.Linear(linear_in_dim, 6 * self.T)
        #barrack
        self.linear4 = nn.Linear(linear_in_dim, 2 * self.T)

    def forward(self, x):
        s, r0, r1 = x["s"], x["r0"], x["r1"]
        output = self.net(self._var(s), self._var(r0), self._var(r1))
        pred = []
        pred.append(self.linear1(output))
        pred.append(self.linear2(output))
        pred.append(self.linear3(output))
        pred.append(self.linear4(output))
        preds = { "%s_T%d" % (k, t) : pred[i][:, t * self.lengths[i] : (t + 1) * self.lengths[i]] for i, k in enumerate(self.outputs) for t in range(self.T) }
        return pred, preds

# Format: key, [model, method]
# if method is None, fall back to default mapping from key to method
Models = {
    "actor_critic": [Model_ActorCritic, ActorCritic],
    "actor_critic_actionmap": [Model_ActorCritic_ActionMap, ActorCritic],
    "Q_learning": [Model_Q, Q_learning],
    "forward": [Model_Forward, ForwardPrediction],
    "forward_sample": [Model_Forward_Sample, ForwardPredictionSample],
}
