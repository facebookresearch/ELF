# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import torch
import torch.nn as nn
from torch.autograd import Variable
import math

from rlpytorch import ArgsProvider, PolicyGradient, DiscountedReward, ValueMatcher, add_err

# Actor critic model.
class ActorCriticChanged:
    def __init__(self):
        self.pg = PolicyGradient()
        self.discounted_reward = DiscountedReward()
        self.value_matcher = ValueMatcher()

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("fixed_policy", dict(action="store_true")),
                ("h_match_policy", dict(action="store_true")),
                ("h_match_action", dict(action="store_true")),
                ("h_smooth", dict(action="store_true")),
                ("contrastive_V", dict(action="store_true")),
            ],
            more_args = ["num_games", "batchsize", "min_prob"],
            child_providers = [ self.pg.args, self.discounted_reward.args, self.value_matcher.args ],
        )

        self.prediction_loss = nn.SmoothL1Loss().cuda()
        self.policy_match_loss = nn.SmoothL1Loss().cuda()
        self.policy_max_action_loss = nn.NLLLoss().cuda()
        self.rank_loss = nn.MarginRankingLoss().cuda()

    def update(self, mi, batch, stats):
        ''' Actor critic model '''
        m = mi["model"]
        args = self.args

        T = batch["a"].size(0)

        state_curr = m(batch.hist(T - 1))

        self.discounted_reward.setR(state_curr["V"].squeeze().data, stats)

        next_h = state_curr["h"].data
        policies = [0] * T
        policies[T - 1] = state_curr["pi"].data

        for t in range(T - 2, -1, -1):
            bht = batch.hist(t)
            state_curr = m.forward(bht)

            # go through the sample and get the rewards.
            a = batch["a"][t]
            V = state_curr["V"].squeeze()

            R = self.discounted_reward.feed(
                dict(r=batch["r"][t], terminal=batch["terminal"][t]),
                stats=stats)

            pi = state_curr["pi"]
            policies[t] = pi.data

            overall_err = None

            if not args.fixed_policy:
                overall_err = self.pg.feed(R - V.data, state_curr, bht, stats, old_pi_s=bht)
                overall_err += self.value_matcher.feed(dict(V=V, target=R), stats)

            if args.h_smooth:
                curr_h = state_curr["h"]
                # Block gradient
                curr_h = Variable(curr_h.data)
                future_pred = m.transition(curr_h, a)
                pred_h = future_pred["hf"]
                predict_err = self.prediction_loss(pred_h, Variable(next_h))
                overall_err = add_err(overall_err, predict_err)

                stats["predict_err"].feed(predict_err.data[0])

                if args.contrastive_V:
                    # Sample an action other than the current action.
                    prob = pi.data.clone().fill_(1 / (pi.size(1) - 1))
                    # Make the selected entry zero.
                    prob.scatter_(1, a.view(-1, 1), 0.0)
                    other_a = prob.multinomial(1)
                    other_future_pred = m.transition(curr_h, other_a)
                    other_pred_h = other_future_pred["hf"]

                    # Make sure the predicted values are lower than the gt
                    # one (we might need to add prob?)
                    # Stop the gradient.
                    pi_V = m.decision(pred_h.data)
                    pi_V_other = m.decision(other_pred_h.data)
                    all_one = R.clone().view(-1, 1).fill_(1.0)

                    rank_err = self.rank_loss(pi_V["V"], pi_V_other["V"], Variable(all_one))
                    value_err = self.prediction_loss(pi_V["V"], Variable(R))

                    stats["rank_err"].feed(rank_err.data[0])
                    stats["value_err"].feed(value_err.data[0])

                    overall_err = add_err(overall_err, rank_err)
                    overall_err = add_err(overall_err, value_err)

            if overall_err is not None:
                overall_err.backward()

            next_h = state_curr["h"].data

            if overall_err is not None:
                stats["cost"].feed(overall_err.data[0])
            #print("[%d]: reward=%.4f, sum_reward=%.2f, acc_reward=%.4f, value_err=%.4f, policy_err=%.4f" % (i, r.mean(), r.sum(), R.mean(), value_err.data[0], policy_err.data[0]))

        if args.h_match_policy or args.h_match_action:
            state_curr = m.forward(batch.hist(0))
            h = state_curr["h"]
            if args.fixed_policy:
                h = Variable(h.data)

            total_policy_err = None
            for t in range(0, T - 1):
                # forwarded policy should be identical with current policy
                V_pi = m.decision_fix_weight(h)
                a = batch["a"][t]
                pi_h = V_pi["pi"]

                # Nothing to learn when t = 0
                if t > 0:
                    if args.h_match_policy:
                        policy_err = self.policy_match_loss(pi_h, Variable(policies[t]))
                        stats["policy_match_err%d" % t].feed(policy_err.data[0])
                    elif args.h_match_action:
                        # Add normalization constant
                        logpi_h = (pi_h + args.min_prob).log()
                        policy_err = self.policy_max_action_loss(logpi_h, Variable(a))
                        stats["policy_match_a_err%d" % t].feed(policy_err.data[0])

                    total_policy_err = add_err(total_policy_err, policy_err)

                future_pred = m.transition(h, a)
                h = future_pred["hf"]

            total_policy_err.backward()
            stats["total_policy_match_err"].feed(total_policy_err.data[0])
