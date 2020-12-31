# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import torch
import torch.nn as nn
from rlpytorch import Model
from collections import Counter

class MiniRTSNet(Model):
    def __init__(self, args, output1d=True):
        # this is the place where you instantiate all your modules
        # you can later access them using the same names you've given them in here
        super(MiniRTSNet, self).__init__(args)
        self._init(args)
        self.output1d = output1d
        #self.isPrint = False

    def _init(self, args):
        self.m = args.params.get("num_planes_per_time_stamp", 13)
        self.input_channel = args.params.get("num_planes", self.m)
        self.mapx = args.params.get("map_x", 20)
        self.mapy = args.params.get("map_y", 20)
        self.pool = nn.MaxPool2d(2, 2)

        # self.arch = "ccpccp"
        # self.arch = "ccccpccccp"
        if self.args.arch[0] == "\"" and self.args.arch[-1] == "\"":
            self.arch = self.args.arch[1:-1]
        else:
            self.arch = self.args.arch
        self.arch, channels = self.arch.split(";")

        self.num_channels = []
        for i, v in enumerate(channels.split(",")):
            if v == "-":
                self.num_channels.append(self.m if i > 0 else self.input_channel)
            else:
                self.num_channels.append(int(v))

        self.convs = [ nn.Conv2d(self.num_channels[i], self.num_channels[i+1], 3, padding = 1) for i in range(len(self.num_channels)-1) ]
        for i, conv in enumerate(self.convs):
            setattr(self, "conv%d" % (i + 1), conv)

        self.relu = nn.ReLU() if self._no_leaky_relu() else nn.LeakyReLU(0.1)

        if not self._no_bn():
            self.convs_bn = [ nn.BatchNorm2d(conv.out_channels) for conv in self.convs ]
            for i, conv_bn in enumerate(self.convs_bn):
                setattr(self, "conv%d_bn" % (i + 1), conv_bn)

    def _no_bn(self):
        return getattr(self.args, "disable_bn", False)

    def _no_leaky_relu(self):
        return getattr(self.args, "disable_leaky_relu", False)

    def get_define_args():
        return [
            ("arch", "ccpccp;-,64,64,64,-")
        ]

    def forward(self, input):
        # BN and LeakyReLU are from Wendy's code.
        x = input.view(input.size(0), self.input_channel, self.mapy, self.mapx)
        # if not self.isPrint:
        #     print("input size: ",input.size())
        #     print("input: ",input)
        #     print("x size:",x.size())
        #     print("x: ",x)
        #     print("Net",self)
        #     self.isPrint = True
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

        if self.output1d:
            x = x.view(x.size(0), -1)
        
        # if not self.isPrint:
        #     print("x",x)
        #     print("x.size ",x.size())
        #     self.isPrint = True
        
        return x     # 64 x 550
