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
from .args_utils import ArgsProvider
from .rlmethod_utils import *

class PolicyGradient:
    def __init__(self, args=None):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("entropy_ratio", dict(type=float, help="The entropy ratio we put on PG", default=0.01)),
                ("grad_clip_norm", dict(type=float, help="Gradient norm clipping", default=None)),
                ("min_prob", dict(type=float, help="Minimal probability used in training", default=1e-6)),
                ("ratio_clamp", 10),
            ],
            on_get_args = self._init,
            fixed_args = args,
        )

    def _init(self, args):
        self.policy_loss = nn.NLLLoss().cuda()

    def _compute_one_policy_entropy_err(self, pi, a):
        batchsize = a.size(0)

        # Add normalization constant
        logpi = (pi + self.args.min_prob).log()

        # Get policy. N * #num_actions
        policy_err = self.policy_loss(logpi, a)
        entropy_err = (logpi * pi).sum() / batchsize
        return dict(policy_err=policy_err, entropy_err=entropy_err)

    def _compute_policy_entropy_err(self, pi, a):
        args = self.args

        errs = { }
        if isinstance(pi, list):
            # Action map, and we need compute the error one by one.
            for i, pix in enumerate(pi):
                for j, pixy in enumerate(pix):
                    errs = accumulate(errs, self._compute_one_policy_entropy_err(pixy, a[:,i,j], args.min_prob))
        else:
            errs = self._compute_one_policy_entropy_err(pi, a)

        return errs

    def _reg_backward(self, v, pg_weights):
        grad_clip_norm = getattr(self.args, "grad_clip_norm", None)
        def bw_hook(grad_in):
            # this works only on pytorch 0.2.0
            grad = grad_in.clone()
            grad.mul_(pg_weights.view(-1, 1))
            if grad_clip_norm is not None:
                average_norm_clip(grad, grad_clip_norm)
            return grad
        v.register_hook(bw_hook)

    def feed(self, batch, stats):
        '''
        One iteration of policy gradient. pho nabla_w log p_w(a|s) Q + entropy_ratio * nabla H(pi(.|s))
        Keys:
            Q (tensor): estimated return
            a (tensor): action
            pi (variable): policy
            old_pi (tensor, optional): old policy, in order to get importance factor.
        '''
        args = self.args
        Q = batch["Q"]
        a = batch["a"]
        pi = batch["pi"]
        batchsize = Q.size(0)

        # We need to set it beforehand.
        # Note that the samples we collect might be off-policy, so we need
        # to do importance sampling.
        pg_weights = Q.clone()

        if "old_pi" in batch:
            old_pi = batch["old_pi"]
            # Cap it.
            coeff = torch.clamp(pi.data.div(old_pi), max=args.ratio_clamp).gather(1, a.view(-1, 1)).squeeze()
            pg_weights.mul_(coeff)
            # There is another term (to compensate clamping), but we omit it for now.

        # Compute policy gradient error:
        errs = self._compute_policy_entropy_err(pi, Variable(a))
        policy_err = errs["policy_err"]
        entropy_err = errs["entropy_err"]

        self._reg_backward(policy_err, pg_weights)

        if stats is not None:
            stats["policy_err"].feed(policy_err.data[0])
            stats["entropy_err"].feed(entropy_err.data[0])

        return policy_err + entropy_err * args.entropy_ratio

