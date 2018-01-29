# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import torch
import torch.nn as nn
from torch.autograd import Variable

from ..args_provider import ArgsProvider
from .utils import *

class PolicyGradient(object):
    def __init__(self):
        '''Initialization for arguments.
        Accepted arguments:

        ``entropy_ratio``: The entropy ratio we put on PolicyGradient

        ``grad_clip_norm``: Gradient norm clipping

        ``min_prob``: Minimal probability used in training

        ``ratio_clamp``: importance sampling coefficient clamp

        ``policy_action_nodes``: ;separated string that specify policy_action nodes.
        '''
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("entropy_ratio", dict(type=float, help="The entropy ratio we put on PG", default=0.01)),
                ("grad_clip_norm", dict(type=float, help="Gradient norm clipping", default=None)),
                ("min_prob", dict(type=float, help="Minimal probability used in training", default=1e-6)),
                ("ratio_clamp", dict(type=float, help="Importance sampling coefficient clamp", default=10.0)),
                ("policy_action_nodes", dict(type=str, help=";separated string that specify policy_action nodes.", default="pi,a"))
            ],
            on_get_args = self._init,
        )

    def _init(self, args):
        '''Initialize policy loss to be an ``nn.NLLLoss`` and parse ``policy_action_nodes``'''
        self.policy_loss = nn.NLLLoss().cuda()
        self.policy_action_nodes = []
        for node in args.policy_action_nodes.split(";"):
            policy, action = node.split(",")
            self.policy_action_nodes.append((policy, action))

    def _compute_one_policy_entropy_err(self, pi, old_pi, a, Q):
        '''Compute policy error and entropy error for one batch.

        Returns:
            dict of
            ``logpi``: log policy
            ``policy_err``: polict error
            ``entropy_err``: entropy error
        '''
        batchsize = a.size(0)

        # Add normalization constant
        logpi = (pi + self.args.min_prob).log()
        # TODO Seems that logpi.clone() won't create a few hook list.
        # See https://github.com/pytorch/pytorch/issues/2601
        logpi2 = (pi + self.args.min_prob).log()

        # Get policy. N * #num_actions
        policy_err = self.policy_loss(logpi, a)
        entropy_err = (logpi2 * pi).sum() / batchsize
        return dict(logpi=logpi, policy_err=policy_err, entropy_err=entropy_err)

    def _compute_policy_entropy_err(self, pi, old_pi, a, Q):
        '''Compute policy error and entropy error for a batch. Pass in ``args.min_prob`` to avoid ``Nan`` in logrithms.

        Returns:
            dict of
            ``logpi``: log policy
            ``policy_err``: polict error
            ``entropy_err``: entropy error
        '''
        args = self.args

        errs = { }
        if isinstance(pi, list):
            # Action map, and we need compute the error one by one.
            for i, pix in enumerate(pi):
                for j, pixy in enumerate(pix):
                    errs = accumulate(
                        errs,
                        self._compute_one_policy_entropy_err(
                            pixy, old_pi[i][j], a[:,i,j], Q))
        else:
            errs = self._compute_one_policy_entropy_err(pi, old_pi, a, Q)

        return errs

    def _reg_backward(self, v, pg_weights):
        ''' Register the backward hook. Clip the gradient if necessary.'''
        grad_clip_norm = getattr(self.args, "grad_clip_norm", None)
        def bw_hook(grad_in):
            # this works only on pytorch 0.2.0
            grad_weights = pg_weights
            if pg_weights.dim() == 1 and grad_in.dim() == 2:
                grad_weights = pg_weights.view(-1, 1)
            grad = grad_in.mul(grad_weights)

            # import pdb
            # pdb.set_trace()
            if grad_clip_norm is not None:
                average_norm_clip(grad, grad_clip_norm)

            return grad
        v.register_hook(bw_hook)

    def _init_pg_weights(self, Q):
        return Q.clone()

    def feed(self, Q, pi_s, actions, stats, old_pi_s=dict()):
        '''
        One iteration of policy gradient.

        pho nabla_w log p_w(a|s) Q + entropy_ratio * nabla H(pi(.|s))

        Args:
            Q(tensor): estimated return
            actions(tensor): action
            pi_s(variable): policy
            old_pi_s(tensor, optional): old policy, in order to get importance factor.

        If you specify multiple policies, then all the log prob of these policies are added, and their importance factors are multiplied.
        Feed to stats: policy error and nll error

        '''
        args = self.args
        batchsize = Q.size(0)

        # We need to set it beforehand.
        # Note that the samples we collect might be off-policy, so we need
        # to do importance sampling.
        pg_weights = self._init_pg_weights(Q)

        policy_err = None
        entropy_err = None
        log_pi_s = []

        for pi_node, a_node in self.policy_action_nodes:
            pi = pi_s[pi_node]
            a = actions[a_node].squeeze()

            old_pi = None
            if pi_node in old_pi_s:
                old_pi = old_pi_s[pi_node]

                # Cap it.
                coeff = torch.clamp(
                    pi.data.div(old_pi.squeeze()),
                    max=args.ratio_clamp,
                ).gather(1, a.view(-1, 1)).squeeze()
                pg_weights.mul_(coeff)
                # There is another term (to compensate clamping), but we omit it for now.

            # Compute policy gradient error:
            errs = self._compute_policy_entropy_err(
                pi, old_pi, Variable(a, requires_grad=False), Q)
            policy_err = add_err(policy_err, errs["policy_err"])
            entropy_err = add_err(entropy_err, errs["entropy_err"])
            log_pi_s.append(errs["logpi"])

            stats["nll_" + pi_node].feed(errs["policy_err"].data[0])
            stats["entropy_" + pi_node].feed(errs["entropy_err"].data[0])

        for log_pi in log_pi_s:
            self._reg_backward(
                log_pi, Variable(pg_weights, requires_grad=False))

        if len(args.policy_action_nodes) > 1:
            stats["total_nll"].feed(policy_err.data[0])
            stats["total_entropy"].feed(entropy_err.data[0])

        return policy_err + entropy_err * args.entropy_ratio
