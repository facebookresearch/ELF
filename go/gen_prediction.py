import torch
import torch.nn as nn
from torch.autograd import Variable
import math

from rlpytorch import ArgsProvider, add_err
from rlpytorch.trainer import topk_accuracy

class GenPrediction:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("multipred_no_backprop", dict(action="store_true")),
            ],
        )

        self.policy_loss = nn.NLLLoss().cuda()
        self.value_loss = nn.MSELoss().cuda()

    def update(self, mi, batch, stats):
        ''' Update given batch '''
        # Current timestep.
        state_curr = mi["model"](batch.hist(0))
        output = state_curr["output"]
        s = state_curr["s"]
        s = s.view(-1, 361, 35).transpose(1, 2).contiguous().narrow(2, 7, 3)

        loss = self.value_loss(s, output)
        stats["loss"].feed(loss.data[0])

        if not self.args.multipred_no_backprop:
            loss.backward()
