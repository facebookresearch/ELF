from rlpytorch import Model, ActorCritic
from gen_prediction import GenPrediction

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
        self.dim = getattr(args, "dim", 128)
        self.convs = []
        self.convs_bn = []
        last_planes = 1

        for i in range(args.num_layer):
            conv = nn.Conv2d(last_planes, self.dim, 3, padding=1)
            conv_bn = nn.BatchNorm2d(self.dim) if not getattr(args, "no_bn", False) else lambda x: x
            setattr(self, "conv" + str(i), conv)
            self.convs.append(conv)
            setattr(self, "conv_bn" + str(i), conv_bn)
            self.convs_bn.append(conv_bn)
            last_planes = self.dim

        self.final_conv = nn.Conv2d(self.dim, 3, 3, padding=1)

        # Softmax as the final layer
        self.softmax = nn.Softmax()
        self.relu = nn.LeakyReLU(0.1) if not getattr(args, "no_leaky_relu", False) else nn.ReLU()

    def get_define_args():
        return [
            ("no_bn", dict(action="store_true")),
            ("no_leaky_relu", dict(action="store_true")),
            ("num_layer", 19),
            ("dim", 128)
        ]

    def forward(self, x):
        s = self._var(x["s"])
        batchsize = s.size(0)
        z = self._var(torch.rand(batchsize, 1, 19, 19)) #random input for generative model
        for conv, conv_bn in zip(self.convs, self.convs_bn):
            z = conv_bn(self.relu(conv(z)))

        output = self.final_conv(z)
        d = self.board_size * self.board_size
        z = z.view(batchsize, 3, d).transpose(1, 2).contiguous().view(-1, 3)
        output = self.softmax(z).view(batchsize, 19, 19, 3)
        return dict(output=output, pi=[1]*batchsize, s=s)

# Format: key, [model, method]
Models = {
    "df_policy" : [Model_Policy, GenPrediction]
}
