# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

from .sample_methods import sample_multinomial, epsilon_greedy
from ..args_provider import ArgsProvider

class Sampler:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("sample_policy", dict(type=str, choices=["epsilon-greedy", "multinomial", "uniform"], help="Sample policy", default="epsilon-greedy")),
                ("greedy", dict(action="store_true")),
                ("epsilon", dict(type=float, help="Used in epsilon-greedy approach", default=0.00))
            ]
        )

    def sample(self, state_curr):
        if self.args.greedy:
            # print("Use greedy approach")
            action = epsilon_greedy(state_curr, self.args, node="pi")
        else:
            # print("Use multinomial approach")
            action = sample_multinomial(state_curr, self.args, node="pi")
        return action

