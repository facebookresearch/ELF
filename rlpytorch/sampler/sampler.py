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
        ''' Initialization for Sampler. Accepted arguments:

        ``sample_policy``: choices of ``epsilon-greedy``, ``multinomial`` or ``uniform``,

        ``greedy``: if ``True``, pick the action with maximum probability, otherwise sample from it.
        ``epsilon``: Minimum probability in sampling
        ``sample_nodes``: ;-separated nodes to be sampled and saved
        '''
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("sample_policy", dict(type=str, choices=["epsilon-greedy", "multinomial", "uniform"], help="Sample policy", default="epsilon-greedy")),
                ("greedy", dict(action="store_true")),
                ("epsilon", dict(type=float, help="Used in epsilon-greedy approach", default=0.00)),
                ("sample_nodes", dict(type=str, help=";-separated nodes to be sampled and saved", default="pi,a"))
            ],
            on_get_args = self._on_get_args,
        )

    def _on_get_args(self, _):
        self.sample_nodes = []
        for nodes in self.args.sample_nodes.split(";"):
            policy, action = nodes.split(",")
            self.sample_nodes.append((policy, action))

    def sample(self, state_curr):
        ''' Sample an action from distribution using a certain sample method

        Args:
            state_curr(dict): current state containing all data
        '''
        #TODO: This only handles epsilon_greedy and multinomial for now. Add uniform and original_distribution?
        sampler = epsilon_greedy if self.args.greedy else sample_multinomial

        actions = dict()
        for pi_node, a_node in self.sample_nodes:
            actions[a_node] = sampler(state_curr, self.args, node=pi_node)
            actions[pi_node] = state_curr[pi_node].data
        return actions
