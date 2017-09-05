import torch
import torch.nn as nn
from torch.autograd import Variable
import math

from rlpytorch import ArgsProvider, add_err

class MultiplePrediction:
    def __init__(self, args=None):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
            ],
            fixed_args = args,
        )

        self.policy_loss = nn.NLLLoss().cuda()
        self.value_loss = nn.MSELoss().cuda()

    def update(self, mi, batch, stats):
        ''' Update given batch '''
        # Current timestep.
        state_curr = mi["model"](batch.hist(0))
        total_loss = None
        eps = 1e-6
        targets = batch.hist(0)["a"]
        for i, pred in enumerate(state_curr["a"]):
            # backward.
            loss = self.policy_loss((pred + eps).log(), Variable(targets[:, i]))
            stats["loss" + str(i)].feed(loss.data[0])
            total_loss = add_err(total_loss, loss / (i + 1))

        stats["total_loss"].feed(total_loss.data[0])
        total_loss.backward()
