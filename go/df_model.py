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
        self.convs = []
        last_planes = self.num_planes
        for i in range(10):
            conv = nn.Conv2d(last_planes, 64, 3, padding=1)
            setattr(self, "conv" + str(i), conv)
            self.convs.append(conv)
            last_planes = 64

        self.final_conv = nn.Conv2d(64, self.num_future_actions, 3, padding=1)

        # Softmax as the final layer
        self.softmax = nn.Softmax()
        self.relu = nn.ReLU()

    def forward(self, x):
        s = self._var(x["features"])
        for conv in self.convs:
            s = self.relu(conv(s))

        output = self.final_conv(s)
        actions = []
        d = self.board_size * self.board_size
        for i in range(self.num_future_actions):
            actions.append(self.softmax(output[:,i].contiguous().view(-1, d)))
        return output, dict(actions=actions, pi=actions[0], V=actions[0].select(1,0))

# Format: key, [model, method]
Models = {
    "df_policy" : [Model_Policy, MultiplePrediction]
}
