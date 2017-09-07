import torch
import torch.nn as nn
from rlpytorch import Model
from collections import Counter

class MiniRTSNet(Model):
    def __init__(self, args):
        # this is the place where you instantiate all your modules
        # you can later access them using the same names you've given them in here
        super(MiniRTSNet, self).__init__(args)
        self._init(args)

    def _init(self, args):
        self.input_channel = args.params["num_planes"]
        self.m = args.params["num_unit_type"] + 7
        self.mapx = args.params["map_x"]
        self.mapy = args.params["map_y"]
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

    def forward(self, input, res):
        # BN and LeakyReLU are from Wendy's code.
        x = input.view(input.size(0), self.input_channel, self.mapy, self.mapx)

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


