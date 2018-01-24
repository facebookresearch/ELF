import torch
import torch.nn as nn
from torch.autograd import Variable
import math

from rlpytorch import ArgsProvider, add_err
from rlpytorch.trainer import topk_accuracy

class MCTSPrediction:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("multipred_no_backprop", dict(action="store_true")),
            ],
        )

        self.policy_loss = nn.KLDivLoss().cuda()
        self.value_loss = nn.MSELoss().cuda()

    def update(self, mi, batch, stats):
        ''' Update given batch '''
        # Current timestep.
        state_curr = mi["model"](batch.hist(0))
        eps = 1e-6
        targets = batch.hist(0)["mcts_scores"]
        logpi = state_curr["logpi"]
        # backward.
        loss = self.policy_loss(logpi, Variable(targets + eps)) * logpi.size(1)
        stats["loss"].feed(loss.data[0])
        total_policy_loss = loss

        total_value_loss = None
        if "V" in state_curr and "winner" in batch:
            total_value_loss = self.value_loss(state_curr["V"], Variable(batch.hist(0)["winner"]))

        stats["total_policy_loss"].feed(total_policy_loss.data[0])
        if total_value_loss is not None:
            stats["total_value_loss"].feed(total_value_loss.data[0])
            total_loss = total_policy_loss + total_value_loss
        else:
            total_loss = total_policy_loss

        stats["total_loss"].feed(total_loss.data[0])
        if not self.args.multipred_no_backprop:
            total_loss.backward()
