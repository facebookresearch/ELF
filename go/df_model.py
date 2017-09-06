from rlpytorch import Model, ActorCritic
from multiple_prediction import MultiplePrediction

import torch
import torch.nn as nn

class Model_Policy(Model):
    def __init__(self, args):
        super(Model_Policy, self).__init__(args)

        params = args.params

        self.board_size = params["board_size"]
        self.num_future_actions = params["num_future_actions"]
        self.num_planes = params["num_planes"]
        # print("#future_action: " + str(self.num_future_actions))
        # print("#num_planes: " + str(self.num_planes))

        # Simple method. multiple conv layers.
        self.dim = 128
        self.convs = []
        self.convs_bn = []
        last_planes = self.num_planes
        for i in range(10):
            conv = nn.Conv2d(last_planes, self.dim, 3, padding=1)
            conv_bn = nn.BatchNorm2d(self.dim)
            setattr(self, "conv" + str(i), conv)
            setattr(self, "conv_bn" + str(i), conv_bn)
            self.convs.append(conv)
            self.convs_bn.append(conv_bn)
            last_planes = self.dim

        self.final_conv = nn.Conv2d(self.dim, self.num_future_actions, 3, padding=1)

        # Softmax as the final layer
        self.softmax = nn.Softmax()
        self.relu = nn.LeakyReLU(0.1)

    def forward(self, x):
        s = self._var(x["features"])
        for conv, conv_bn in zip(self.convs, self.convs_bn):
            s = conv_bn(conv(s))

        output = self.final_conv(s)
        actions = []
        d = self.board_size * self.board_size
        for i in range(self.num_future_actions):
            actions.append(self.softmax(output[:,i].contiguous().view(-1, d)))
        return dict(a=actions)

# Format: key, [model, method]
Models = {
    "df_policy" : [Model_Policy, MultiplePrediction]
}
