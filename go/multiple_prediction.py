import torch
import torch.nn as nn
from torch.autograd import Variable
import math

from rlpytorch import ArgsProvider, add_err

class MultiplePrediction:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
            ],
        )

        self.policy_loss = nn.NLLLoss().cuda()
        self.value_loss = nn.MSELoss().cuda()

    def accuracy(self, output, target, topk=(1,)):
        """Computes the precision@k for the specified values of k"""
        maxk = max(topk)
        batch_size = target.size(0)

        _, pred = output.topk(maxk, 1, True, True)
        pred = pred.t()
        correct = pred.eq(target.view(1, -1).expand_as(pred))

        res = []
        for k in topk:
            correct_k = correct[:k].view(-1).float().sum(0)
            res.append(correct_k.mul_(100.0 / batch_size))
        return res

    def update(self, mi, batch, stats):
        ''' Update given batch '''
        # Current timestep.
        state_curr = mi["model"](batch.hist(0))
        total_loss = None
        eps = 1e-6
        targets = batch.hist(0)["a"]
        for i, pred in enumerate(state_curr["a"]):
            if i == 0:
                prec1, prec5 = self.accuracy(pred.data, targets[:, i].contiguous(), topk=(1, 5))
                stats["top1_acc"].feed(prec1[0])
                stats["top5_acc"].feed(prec5[0])
            # backward.
            loss = self.policy_loss((pred + eps).log(), Variable(targets[:, i]))
            stats["loss" + str(i)].feed(loss.data[0])
            total_loss = add_err(total_loss, loss / (i + 1))

        stats["total_loss"].feed(total_loss.data[0])
        total_loss.backward()
