import torch
import torch.nn as nn

from model_base import *
from rlmethod_forward import ForwardPrediction, ForwardPredictionSample
from rlmethod_common import ActorCritic, Q_learning

class MiniRTSNet(Model):
    def __init__(self, args):
        # this is the place where you instantiate all your modules
        # you can later access them using the same names you've given them in here
        super(MiniRTSNet, self).__init__(args)
        self.m = 2
        self.conv1 = nn.Conv2d(self.m, self.m, 3, padding = 1)
        self.conv2 = nn.Conv2d(self.m, self.m, 3, padding = 1)
        self.conv3 = nn.Conv2d(self.m, self.m, 3, padding = 1)
        self.conv4 = nn.Conv2d(self.m, self.m, 3, padding = 1)
        self.relu = nn.ReLU() if self._no_leaky_relu() else nn.LeakyReLU(0.1)

        if not self._no_bn():
            self.conv1_bn = nn.BatchNorm2d(self.m)
            self.conv2_bn = nn.BatchNorm2d(self.m)
            self.conv3_bn = nn.BatchNorm2d(self.m)
            self.conv4_bn = nn.BatchNorm2d(self.m)

    def _no_bn(self):
        return getattr(self.args, "disable_bn", False)

    def _no_leaky_relu(self):
        return getattr(self.args, "disable_leaky_relu", False)

    def forward(self, input):
        # BN and LeakyReLU are from Wendy's code.
        h1 = self.conv1(input.view(input.size(0), self.m, 20, 20))
        if not self._no_bn(): h1 = self.conv1_bn(h1)
        h1 = self.relu(h1)

        h2 = self.conv2(self.relu(h1))
        if not self._no_bn(): h2 = self.conv2_bn(h2)
        h2 = self.relu(h2)

        h3 = self.conv3(self.relu(h2))
        if not self._no_bn(): h3 = self.conv3_bn(h3)
        h3 = self.relu(h3)

        h4 = self.conv4(self.relu(h3))
        if not self._no_bn(): h4 = self.conv4_bn(h4)
        x = h4.view(h4.size(0), -1)
        return x

class Model_ActorCritic(Model):
    def __init__(self, args):
        super(Model_ActorCritic, self).__init__(args)

        params = args.params
        assert isinstance(params["num_action"], int), "num_action has to be a number. action = " + str(params["num_action"])
        self.params = params
        self.net = MiniRTSNet(args)
        linear_in_dim = 800
        self.linear_policy = nn.Linear(linear_in_dim, params["num_action"])
        self.linear_value = nn.Linear(linear_in_dim, 1)
        self.softmax = nn.Softmax()

    def forward(self, x):
        s = x["s"]
        #print(s.size())
        #print(s.type())
        output = self.net(self._var(s))
        policy = self.softmax(self.linear_policy(output))
        value = self.linear_value(output)
        return value, dict(V=value, pi=policy)

    def test(self):
        x = torch.cuda.FloatTensor(max_s, max_s)
        x.fill_(0)
        for i in range(max_s):
            x[i, i] = 1

        _, res = self(self._var(x))
        # Check both policy and value function
        print(res["pi"].exp())
        print(res["V"])

# Format: key, [model, method]
# if method is None, fall back to default mapping from key to method
Models = {
    "actor_critic": [Model_ActorCritic, ActorCritic]
}
