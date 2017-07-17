# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

from .rlmethod_base import *
import torch
import torch.nn as nn
from torch.autograd import Variable
import math

# Methods.

# Actor critic model.
class ActorCritic(LearningMethod):
    def _args(self):
        return [
            ("entropy_ratio", dict(type=float, help="The entropy ratio we put on A3C", default=0.01)),
            ("grad_clip_norm", dict(type=float, help="Gradient norm clipping", default=None)),
            ("discount", dict(type=float, default=0.99)),
            ("min_prob", dict(type=float, help="Minimal probability used in training", default=1e-6)),
        ]

    def _init(self, args):
        self.policy_loss = nn.NLLLoss().cuda()
        self.value_loss = nn.SmoothL1Loss().cuda()

        grad_clip_norm = getattr(args, "grad_clip_norm", None)

        self.policy_gradient_weights = None

        def _policy_backward(layer, grad_input, grad_output):
            if self.policy_gradient_weights is None: return
            # Multiply gradient weights
            grad_input[0].data.mul_(self.policy_gradient_weights.expand_as(grad_input[0]))
            if grad_clip_norm is not None:
                average_norm_clip(grad_input[0], grad_clip_norm)

        def _value_backward(layer, grad_input, grad_output):
            if grad_clip_norm is not None:
                average_norm_clip(grad_input[0], grad_clip_norm)

        # Backward hook for training.
        self.policy_loss.register_backward_hook(_policy_backward)
        self.value_loss.register_backward_hook(_value_backward)

    def _compute_one_policy_entropy_err(self, pi, a, min_prob):
        batchsize = a.size(0)

        # Add normalization constant
        logpi = (pi + min_prob).log()

        # Get policy. N * #num_actions
        policy_err = self.policy_loss(logpi, Variable(a))
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
            errs = self._compute_one_policy_entropy_err(pi, a, args.min_prob)

        return errs

    def update(self, batch):
        ''' Actor critic model '''
        model_interface = self.model_interface
        args = self.args
        T = len(batch)
        bt = batch[T - 1]

        state_curr = model_interface.forward("model", bt)
        R = state_curr["V"].data
        batchsize = R.size(0)

        for i, terminal in enumerate(bt["terminal"]):
            if terminal:
                R[i] = bt["r"][i]

        self.stats["init_reward"].feed(R.mean())
        ratio_clamp = 10

        for t in range(T - 2, -1, -1):
            bt = batch[t]

            # go through the sample and get the rewards.
            a = bt["a"]
            r = bt["r"]

            state_curr = model_interface.forward("model", bt)

            # Compute the reward.
            R = R * args.discount + r
            # If we see any terminal signal, break the reward backpropagation chain.
            for i, terminal in enumerate(bt["terminal"]):
                if terminal:
                    R[i] = r[i]

            pi = state_curr["pi"]
            old_pi = bt["pi"]
            V = state_curr["V"]

            # We need to set it beforehand.
            # Note that the samples we collect might be off-policy, so we need
            # to do importance sampling.
            self.policy_gradient_weights = R - V.data

            # Cap it.
            coeff = torch.clamp(pi.data.div(old_pi), max=ratio_clamp).gather(1, a.view(-1, 1))
            self.policy_gradient_weights.mul_(coeff)
            # There is another term (to compensate clamping), but we omit it for
            # now.

            # Compute policy gradient error:
            errs = self._compute_policy_entropy_err(pi, a)
            policy_err = errs["policy_err"]
            entropy_err = errs["entropy_err"]

            # Compute critic error
            value_err = self.value_loss(V, Variable(R))

            overall_err = policy_err + entropy_err * args.entropy_ratio + value_err
            overall_err.backward()

            self.stats["rms_advantage"].feed(self.policy_gradient_weights.norm() / math.sqrt(batchsize))
            self.stats["cost"].feed(overall_err.data[0])
            self.stats["predict_reward"].feed(V.data[0].mean())
            self.stats["reward"].feed(r.mean())
            self.stats["acc_reward"].feed(R.mean())
            self.stats["value_err"].feed(value_err.data[0])
            self.stats["policy_err"].feed(policy_err.data[0])
            self.stats["entropy_err"].feed(entropy_err.data[0])
            #print("[%d]: reward=%.4f, sum_reward=%.2f, acc_reward=%.4f, value_err=%.4f, policy_err=%.4f" % (i, r.mean(), r.sum(), R.mean(), value_err.data[0], policy_err.data[0]))

class MultiplePrediction(LearningMethod):
    def _init(self, args):
        self.policy_loss = nn.NLLLoss().cuda()
        self.value_loss = nn.MSELoss().cuda()

    def _params(self):
        return [
            ("dummy", 0)
        ]

    def update(self, batch):
        ''' Update given batch '''
        # Current timestep.
        state_curr = self.model_interface.forward("model", batch[0])
        total_loss = None
        eps = 1e-6
        targets = batch[0]["actions"]
        for i, pred in enumerate(state_curr["actions"]):
            # backward.
            loss = self.policy_loss((pred + eps).log(), Variable(targets[:, i]))
            self.stats["loss" + str(i)].feed(loss.data[0])
            if total_loss is None: total_loss = loss
            else: total_loss += loss / (i + 1)

        self.stats["total_loss"].feed(total_loss.data[0])
        total_loss.backward()

CommonMethods = {
    "actor_critic": ActorCritic,
    "multiple_prediction": MultiplePrediction
}
