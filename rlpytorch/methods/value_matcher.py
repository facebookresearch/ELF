# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import torch
import torch.nn as nn
from torch.autograd import Variable
import math
from ..args_provider import ArgsProvider
from .utils import *

class ValueMatcher:
    def __init__(self):
        ''' Initialize value matcher.
        Accepted arguments:

        ``grad_clip_norm`` : Gradient norm clipping

        ``value_node``:  name of the value node
        '''
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("grad_clip_norm", dict(type=float, help="Gradient norm clipping", default=None)),
                ("value_node", dict(type=str, help="The name of the value node", default="V"))
            ],
            on_get_args = self._init,
        )

    def _init(self, _):
        ''' Initialize value loss to be ``nn.SmoothL1Loss``. '''
        self.value_loss = nn.SmoothL1Loss().cuda()
        self.value_node = self.args.value_node

    def _reg_backward(self, v):
        ''' Register the backward hook. Clip the gradient if necessary.'''
        grad_clip_norm = getattr(self.args, "grad_clip_norm", None)
        if grad_clip_norm:
            def bw_hook(grad_in):
                grad = grad_in.clone()
                if grad_clip_norm is not None:
                    average_norm_clip(grad, grad_clip_norm)
                return grad

            v.register_hook(bw_hook)

    def feed(self, batch, stats):
        '''
        One iteration of value match.

        nabla_w Loss(V - target)

        Keys in a batch:

        ``V`` (variable): value

        ``target`` (tensor): target value.

        Inputs that are of type Variable can backpropagate.

        Feed to stats: predicted value and value error

        Returns:
            value_err
        '''
        V = batch[self.value_node]
        value_err = self.value_loss(V, Variable(batch["target"]))
        self._reg_backward(V)
        stats["predicted_" + self.value_node].feed(V.data[0])
        stats[self.value_node + "_err"].feed(value_err.data[0])

        return value_err
