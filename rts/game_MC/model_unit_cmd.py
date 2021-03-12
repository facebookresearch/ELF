# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import torch
import torch.nn as nn
from copy import deepcopy
from collections import Counter

from rlpytorch import Model, ActorCritic
from actor_critic_changed import ActorCriticChanged
from trunk import MiniRTSNet

def flattern(x):
    return x.view(x.size(0), -1)


class Model_ActorCritic(Model):
    def __init__(self, args):
        super(Model_ActorCritic, self).__init__(args)
        self._init(args)

    def _init(self, args):
        params = args.params
        assert isinstance(params["num_action"], int), "num_action has to be a number. action = " + str(params["num_action"])
        self.params = params
        self.net = MiniRTSNet(args, output1d=False)

        self.na = params["num_action"]
        self.num_unit = params["num_unit_type"]
        self.num_planes = params["num_planes"]
        self.num_cmd_type = params["num_cmd_type"]
        self.mapx = params["map_x"]
        self.mapy = params["map_y"]

        out_dim = self.num_planes * self.mapx * self.mapy

        # After trunk, we can predict the unit action and value.
        # Four dimensions. unit loc, target loc,
        self.unit_locs = nn.Conv2d(self.num_planes, 1, 3, padding=1)
        self.target_locs = nn.Conv2d(self.num_planes, 1, 3, padding=1)
        
        # test
        self.unit_select = nn.Linear(self.mapx * self.mapy,16)
        self.target_select = nn.Linear(self.mapx * self.mapy,12)

        #self.cmd_types = nn.Linear(out_dim, self.num_cmd_type)
        self.cmd_types = nn.Linear(out_dim, 3)

        #self.build_types = nn.Linear(out_dim, self.num_unit)
        self.value = nn.Linear(out_dim, 1)

        self.relu = nn.LeakyReLU(0.1)
        self.softmax = nn.Softmax()

    def get_define_args():
        return MiniRTSNet.get_define_args()

    def forward(self, x):
        #s, res = x["s"], x["res"]
        # output = self.net(self._var(x["s"]), self._var(x["res"]))
        # import pdb
        # pdb.set_trace()
        s = x["s"]
        output = self.net(self._var(x["s"])) # 8 10 70 70
 
        # unit_locs = self.softmax(flattern(self.unit_locs(output)))   # 18
        # target_locs = self.softmax(flattern(self.target_locs(output))) # 8
        unit_locs = self.softmax(self.unit_select(flattern(self.unit_locs(output))) )   # 16
        target_locs = self.softmax( self.target_select(flattern(self.target_locs(output))) ) # 12

        flat_output = flattern(output)
        cmd_types = self.softmax(self.cmd_types(flat_output))
        #build_types = self.softmax(self.build_types(flat_output))
        value = self.value(flat_output)

        # import pdb
        # pdb.set_trace()
        return dict(V=value, uloc_prob=unit_locs, tloc_prob=target_locs, ct_prob = cmd_types,action_type=1)

# Format: key, [model, method]
# if method is None, fall back to default mapping from key to method
Models = {
    "actor_critic": [Model_ActorCritic, ActorCritic],
}

Defaults = {
    #"sample_nodes": "uloc_prob,uloc;tloc_prob,tloc;ct_prob,ct;bt_prob,bt",
    #"policy_action_nodes": "uloc_prob,uloc;tloc_prob,tloc;ct_prob,ct;bt_prob,bt",
    "sample_nodes": "uloc_prob,uloc;tloc_prob,tloc;ct_prob,ct",
    "policy_action_nodes": "uloc_prob,uloc;tloc_prob,tloc;ct_prob,ct",
    "arch" : "cccc;-,64,64,64,-"
}
