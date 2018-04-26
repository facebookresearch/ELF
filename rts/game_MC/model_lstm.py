# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import torch
import torch.nn as nn
from copy import deepcopy
from collections import Counter
import random

from rlpytorch import Model, RNNActorCritic
from trunk import MiniRTSNet

class Model_RNNActorCritic(Model):
    def __init__(self, args):
        super(Model_RNNActorCritic, self).__init__(args)
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

        self.na = params["num_action"]

        self.linear_policy = nn.Linear(linear_in_dim, self.na)
        self.linear_value = nn.Linear(linear_in_dim, 1)

        self.relu = nn.LeakyReLU(0.1)

        if self.args.concat:
            self.Wt = nn.Linear(linear_in_dim * 2, linear_in_dim)
        else:
            self.Wt = nn.Linear(linear_in_dim, linear_in_dim)

        self.sigmoid = nn.Sigmoid()

        # Adaptive gating
        if getattr(args, "gating", False):
            self.gate_1 = nn.Linear(linear_in_dim, linear_in_dim)
            self.gate_2 = nn.Linear(linear_in_dim, 1)

        self.transition1 = nn.Linear(linear_in_dim + self.na, linear_in_dim)
        self.transition2 = nn.Linear(linear_in_dim, linear_in_dim)
        self.num_hidden_dim = linear_in_dim

        self.softmax = nn.Softmax()

    def get_define_args():
        return MiniRTSNet.get_define_args() + [
            ("ratio_skip_observation", 0.0),
            ("concat", dict(action="store_true")),
            ("enable_transition_model", dict(action="store_true")),
            ("gating", dict(action="store_true")),
        ]

    def _trunk(self, x):
        if self.params.get("model_no_spatial", False):
            # Replace a complicated network with a simple retraction.
            # Input: batchsize, channel, height, width
            xreduced = x["s"].sum(2).sum(3).squeeze()
            xreduced[:, self.num_unit:] /= 20 * 20
            output = self._var(xreduced)
        else:
            s, res = x["s"], x["res"]
            output = self.net(self._var(x["s"]), self._var(x["res"]))

        return output

    def _merge(self, x, hiddens, skip_mat):
        output = self._trunk(x)
        if self.args.gating:
            # Smart selection.
            skip_mat = self.sigmoid(self.gate_2(self.relu(self.gate_1(output))))

        if self.args.concat:
            if self.args.ratio_skip_observation == 0.0:
                output_part = output
            else:
                output_part = output.mul(1.0 - skip_mat)
            h = torch.cat((hiddens, output_part), 1)
        else:
            hidden_part = hiddens.mul(skip_mat)
            output_part = output.mul(1.0 - skip_mat)
            h = hidden_part + output_part
        return self.sigmoid(self.Wt(h))

    def forward(self, x, hiddens):
        batchsize = x["s"].size(0)
        if not hasattr(self, "prob"):
            self.prob = x["res"].clone().resize_(2)
            self.prob[0] = 1 - self.args.ratio_skip_observation
            self.prob[1] = self.args.ratio_skip_observation

        skip_mat = self._var(torch.multinomial(self.prob, batchsize, replacement=True).float().view(-1, 1))

        output = self._merge(x, hiddens, skip_mat)
        return self.decision(output)

    def transition(self, h, a):
        ''' A transition model that could predict the future given the current state and its action '''
        if not self.args.enable_transition_model:
            return h

        na = self.params["num_action"]
        a_onehot = h.data.clone().resize_(a.size(0), na).zero_()
        a_onehot.scatter_(1, a.view(-1, 1), 1)
        tr_in = torch.cat((h, self._var(a_onehot)), 1)

        tr_in2 = self.relu(self.transition1(tr_in))
        h_next = self.sigmoid(self.transition2(tr_in2))
        return h_next

    def decision(self, h):
        policy = self.softmax(self.linear_policy(h))
        value = self.linear_value(h)
        return dict(h=h, V=value, pi=policy)

# Format: key, [model, method]
# if method is None, fall back to default mapping from key to method
Models = {
    "actor_critic": [Model_RNNActorCritic, RNNActorCritic],
}
