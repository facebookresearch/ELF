# Copyright (c) 2018-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import torch
import torch.nn as nn
from torch.autograd import Variable

from ..args_provider import ArgsProvider
from .actor_critic import ActorCritic
from .discounted_reward import DiscountedReward
from .policy_gradient import PolicyGradient
from .value_matcher import ValueMatcher
from .utils import *

class PPOPolicyObjective(PolicyGradient):
    def __init__(self):
        '''Initialization for arguments.
        Accepted arguments:

        ``policy_obj_clip_eps``: Clip ratios to be within 1 +/- epsilon

        ``policy_obj_kl_initial_coeff``: Initial coeff for the KL adaptive loss

        ``policy_obj_kl_target``: Target KL divergence per update (0 means don't use adaptive kl)

        ``use_old_model``: Whether to use old model or current model as KL/clip reference point

        ``entropy_ratio``: The weight of the entropy bonus

        ``grad_clip_norm``: Gradient norm clipping

        ``min_prob``: Minimal probability used in training

        ``ratio_clamp``: importance sampling coefficient clamp

        ``policy_action_nodes``: ;separated string that specify policy_action nodes.

        '''
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("policy_obj_clip_eps", dict(type=float, help="Clip ratios to be within 1 +/- epsilon", default=0.2)),
                ("policy_obj_kl_initial_coeff", dict(type=float, help="Initial coeff for the KL adaptive loss", default=0.0)),
                ("policy_obj_kl_target", dict(type=float, help="Target KL divergence per update (0 means don't use adaptive kl')", default=0.0)),
                ("use_old_model", dict(action="store_true", help="Whether to use old model or current model as KL/clip reference point")),
                ("entropy_ratio", dict(type=float, help="The entropy ratio we put on PG", default=0.01)),
                ("grad_clip_norm", dict(type=float, help="Gradient norm clipping", default=None)),
                ("min_prob", dict(type=float, help="Minimal probability used in training", default=1e-6)),
                ("ratio_clamp", dict(type=float, help="Importance sampling coefficient clamp", default=10.0)),
                ("policy_action_nodes", dict(type=str, help=";separated string that specify policy_action nodes.", default="pi,a")),
            ],
            on_get_args = self._init,
        )

    def _init(self, args):
        super()._init(args)
        self.use_ratio_clip = args.policy_obj_clip_eps > 1e-30
        self.use_kl = args.policy_obj_kl_initial_coeff > 1e-30
        self.use_kl_adapting = args.policy_obj_kl_target > 1e-30
        self.kl_coeff = args.policy_obj_kl_initial_coeff
        self.kl_loss = nn.KLDivLoss().cuda()

    def _compute_one_policy_entropy_err(self, pi, old_pi, a, adv):
        '''Compute policy error and entropy error for one batch.

        Returns:
            dict of
            ``logpi``: log policy
            ``policy_err``: policy error
            ``entropy_err``: entropy error
        '''
        assert old_pi is not None, \
            'PPO requires knowledge of past policy distributions'

        batchsize = adv.size(0)
        normalized_adv = Variable(
            (adv - adv.mean()) / max(adv.std(), 1e-15),
            requires_grad=False)

        old_pi = old_pi + self.args.min_prob
        old_logpi = old_pi.log()
        old_pi_actions = (old_pi + self.args.min_prob).gather(
            1, a.data.view(-1, 1)).squeeze()

        # policy probabiltiies of the realized actions.
        # we do this convoluted intermediate log thing to facilitate
        # the backprop gradient clipping hook.
        logpi_actions = (pi + self.args.min_prob).gather(
            1, a.view(-1, 1)).squeeze().log()
        pi_actions = logpi_actions.exp()

        # probabilities for kl and entropy
        logpi = (pi + self.args.min_prob).log()

        # Get policy. N * #num_actions
        if self.args.use_old_model:
            pi_ratio = pi_actions.div(Variable(old_pi_actions, requires_grad=False))
        else:
            pi_ratio = pi_actions.div(pi_actions.detach())

        if self.use_ratio_clip:
            policy_err = -torch.min(
                normalized_adv * pi_ratio,
                normalized_adv * torch.clamp(
                    pi_ratio,
                    min=1.0 - self.args.policy_obj_clip_eps,
                    max=1.0 + self.args.policy_obj_clip_eps,
                )
            ).sum() / batchsize
        else:
            policy_err = (normalized_adv * pi_ratio).sum() / batchsize

        if self.use_kl:
            if self.args.use_old_model:
                policy_err += \
                    self.kl_coeff * \
                    self.kl_loss(Variable(old_logpi, requires_grad=False),
                                 logpi)
            else:
                policy_err += self.kl_coeff * self.kl_loss(logpi.detach(),
                                                           logpi)

        entropy_err = (logpi * pi).sum() / batchsize

        return dict(logpi=logpi_actions, policy_err=policy_err, entropy_err=entropy_err)

    def _init_pg_weights(self, Q):
        return torch.ones_like(Q)

    def on_model_update(self):
        # TODO: implement adaptation of KL coeff
        pass


# Actor critic model.
class PPOActorCritic(ActorCritic):
    '''An actor critic model using PPO.'''
    def __init__(self):
        super().__init__()
        self.policy_obj = PPOPolicyObjective()
        self._init_args()
