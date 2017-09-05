# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import torch
import torch.nn as nn
from torch.autograd import Variable
import math
from ..args_provider import ArgsProvider
from .utils import *

class ValueMatcher:
    def __init__(self, args=None):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("grad_clip_norm", dict(type=float, help="Gradient norm clipping", default=None)),
            ],
            on_get_args = self._init,
            fixed_args = args,
        )

    def _init(self, _):
        self.value_loss = nn.SmoothL1Loss().cuda()

    def _reg_backward(self, v):
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
        One iteration of value match. nabla_w Loss(V - target)
        Keys:
            V (variable): value
            target (tensor): target value.
        Inputs that are of type Variable can backpropagate.
        '''
        V = batch["V"]
        value_err = self.value_loss(V, Variable(batch["target"]))
        self._reg_backward(V)
        stats["predicted_V"].feed(V.data[0])
        stats["value_err"].feed(value_err.data[0])

        return value_err
