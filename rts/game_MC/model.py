# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import torch
import torch.nn as nn
from copy import deepcopy
from collections import Counter

from rlpytorch import Model, ActorCritic, PPOActorCritic
from actor_critic_changed import ActorCriticChanged
from forward_predict import ForwardPredict
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
        last_num_channel = self.net.num_channels[-1]

        if self.params.get("model_no_spatial", False):
            self.num_unit = params["num_unit_type"]
            linear_in_dim = last_num_channel
        else:
            linear_in_dim = last_num_channel * 25

        self.linear_policy = nn.Linear(linear_in_dim, params["num_action"])
        self.linear_value = nn.Linear(linear_in_dim, 1)

        self.relu = nn.LeakyReLU(0.1)

        self.Wt = nn.Linear(linear_in_dim + params["num_action"], linear_in_dim)
        self.Wt2 = nn.Linear(linear_in_dim, linear_in_dim)
        self.Wt3 = nn.Linear(linear_in_dim, linear_in_dim)

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
            output = self.net(self._var(x["s"]))

        return self.decision(output)

    def decision(self, h):
        h = self._var(h)
        policy = self.softmax(self.linear_policy(h))
        value = self.linear_value(h)
        return dict(h=h, V=value, pi=policy, action_type=0)

    def decision_fix_weight(self, h):
        # Copy linear policy and linear value
        if not hasattr(self, "fixed"):
            self.fixed = dict()
            self.fixed["linear_policy"] = deepcopy(self.linear_policy)
            self.fixed["linear_value"] = deepcopy(self.linear_value)

        policy = self.softmax(self.fixed["linear_policy"](h))
        value = self.fixed["linear_value"](h)
        return dict(h=h, V=value, pi=policy, action_type=0)

    def transition(self, h, a):
        ''' A transition model that could predict the future given the current state and its action '''
        h = self._var(h)
        na = self.params["num_action"]
        a_onehot = h.data.clone().resize_(a.size(0), na).zero_()
        a_onehot.scatter_(1, a.view(-1, 1), 1)
        input = torch.cat((h, self._var(a_onehot)), 1)

        h2 = self.relu(self.Wt(input))
        h3 = self.relu(self.Wt2(h2))
        h4 = self.relu(self.Wt3(h3))

        return dict(hf=h4)

    def reset_forward(self):
        self.Wt.reset_parameters()
        self.Wt2.reset_parameters()
        self.Wt3.reset_parameters()

# Format: key, [model, method]
# if method is None, fall back to default mapping from key to method
Models = {
    "actor_critic": [Model_ActorCritic, ActorCritic],
    "actor_critic_changed": [Model_ActorCritic, ActorCriticChanged],
    "ppo_actor_critic": [Model_ActorCritic, PPOActorCritic],
    "forward_predict": [Model_ActorCritic, ForwardPredict]
}
