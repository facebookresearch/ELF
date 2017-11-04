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

from rlpytorch import ArgsProvider, PolicyGradient, DiscountedReward, ValueMatcher, add_err

class ForwardPredict:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("fixed_policy", dict(action="store_true")),
                ("h_smooth", dict(action="store_true")),
            ],
            more_args = ["num_games", "batchsize", "min_prob"],
            child_providers = [ ],
        )

        self.prediction_loss = nn.SmoothL1Loss().cuda()

    def update(self, mi, batch, stats):
        m = mi["model"]
        args = self.args

        T = batch["a"].size(0)
        total_predict_err = None

        hs = []

        for t in range(0, T - 1):
            # forwarded policy should be identical with current policy
            bht = batch.hist(t)
            state_curr = m(bht)
            if t > 0:
                prev_a = batch["a"][t - 1]
            h = state_curr["h"].data

            for i in range(0, t):
                future_pred = m.transition(hs[i], prev_a)
                pred_h = future_pred["hf"]
                this_err = self.prediction_loss(pred_h, Variable(h))
                total_predict_err = add_err(total_predict_err, this_err)
                hs[i] = pred_h

            term = Variable(1.0 - batch["terminal"][t].float()).view(-1, 1)
            for _h in hs:
                _h.register_hook(lambda grad: grad.mul(term))

            hs.append(Variable(h))

        stats["predict_err"].feed(total_predict_err.data[0])
        total_predict_err.backward()


