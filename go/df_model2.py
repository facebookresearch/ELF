from rlpytorch import Model, ActorCritic
from multiple_prediction import MultiplePrediction

import torch
import torch.nn as nn

class Model_PolicyValue(Model):
    def __init__(self, args):
        super(Model_PolicyValue, self).__init__(args)

        params = args.params

        self.board_size = params["board_size"]
        self.num_future_actions = params["num_future_actions"]
        self.num_planes = params["num_planes"]
        # print("#future_action: " + str(self.num_future_actions))
        # print("#num_planes: " + str(self.num_planes))

        # Simple method. multiple conv layers.
        self.dim = getattr(args, "dim", 128)
        self.convs = []
        self.convs_bn = []
        last_planes = self.num_planes

        for i in range(args.num_layer):
            conv = nn.Conv2d(last_planes, self.dim, 3, padding=1)
            conv_bn = nn.BatchNorm2d(self.dim) if not getattr(args, "no_bn", False) else lambda x: x
            setattr(self, "conv" + str(i), conv)
            self.convs.append(conv)
            setattr(self, "conv_bn" + str(i), conv_bn)
            self.convs_bn.append(conv_bn)
            last_planes = self.dim

        self.final_conv = nn.Conv2d(self.dim, 1, 3, padding=1)

        # Softmax as the final layer
        self.softmax = nn.Softmax()
        self.value = nn.Linear(self.board_size ** 2, 1)
        self.relu = nn.LeakyReLU(0.1) if not getattr(args, "no_leaky_relu", False) else nn.ReLU()

    def get_define_args():
        return [
            ("no_bn", dict(action="store_true")),
            ("no_leaky_relu", dict(action="store_true")),
            ("num_layer", 39),
            ("dim", 128)
        ]

    def forward(self, x):
        s = self._var(x["s"])

        for conv, conv_bn in zip(self.convs, self.convs_bn):
            s = conv_bn(self.relu(conv(s)))

        output = self.final_conv(s)
        d = self.board_size * self.board_size
        h = output.view(-1, d)

        pi = self.softmax(h)
        V = self.value(h)
        return dict(pi=pi, V=V)

# Format: key, [model, method]
Models = {
    "df" : [Model_PolicyValue, MultiplePrediction]
}
