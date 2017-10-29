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

        # Network structure of AlphaGo Zero
        # https://www.nature.com/nature/journal/v550/n7676/full/nature24270.html

        # Simple method. multiple conv layers.
        self.relu = nn.LeakyReLU(0.1) if not getattr(args, "no_leaky_relu", False) else nn.ReLU()
        self.dim = getattr(args, "dim", 128)
        self.convs = []
        last_planes = self.num_planes

        self.init_conv = self._conv_layer(last_planes)

        for i in range(args.num_block):
            conv_lower = self._conv_layer()
            conv_upper = self._conv_layer(relu=False)
            setattr(self, "conv_lower" + str(i), conv_lower)
            setattr(self, "conv_upper" + str(i), conv_upper)

            self.convs.append((conv_lower, conv_upper))

        self.pi_final_conv = self._conv_layer(self.dim, 2, 1)
        self.value_final_conv = self._conv_layer(self.dim, 1, 1)

        d = self.board_size ** 2
        self.pi_linear = nn.Linear(d * 2, d)
        self.value_linear1 = nn.Linear(d, 256)
        self.value_linear2 = nn.Linear(256, 1)

        # Softmax as the final layer
        self.softmax = nn.Softmax()
        self.tanh = nn.Tanh()

    def _conv_layer(self, input_channel=None, output_channel=None, kernel=3, relu=True):
        if input_channel is None:
            input_channel = self.dim
        if output_channel is None:
            output_channel = self.dim

        layers = []
        layers.append(nn.Conv2d(input_channel, output_channel, kernel, padding=kernel // 2))
        if not getattr(self.args, "no_bn", False):
            layers.append(nn.BatchNorm2d(output_channel))
        if relu:
            layers.append(self.relu)

        return nn.Sequential(*layers)

    def get_define_args():
        return [
            ("no_bn", dict(action="store_true")),
            ("no_leaky_relu", dict(action="store_true")),
            ("num_block", 20),
            ("dim", 128)
        ]

    def forward(self, x):
        s = self._var(x["s"])

        s = self.init_conv(s)
        for conv_lower, conv_upper in self.convs:
            s1 = conv_lower(s)
            s1 = conv_upper(s1)
            s1 = s1 + s
            s = self.relu(s1)

        d = self.board_size ** 2

        pi = self.pi_final_conv(s)
        pi = self.pi_linear(pi.view(-1, d * 2))
        pi = self.softmax(pi)

        V = self.value_final_conv(s)
        V = self.relu(self.value_linear1(V.view(-1, d)))
        V = self.value_linear2(V)
        V = self.tanh(V)

        return dict(pi=pi, V=V)

# Format: key, [model, method]
Models = {
    "df" : [Model_PolicyValue, MultiplePrediction]
}
