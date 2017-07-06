# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import abc
from collections import defaultdict
from utils import Stats
from args_utils import ArgsProvider

class LearningMethod:
    def __init__(self, mi=None, args=None):
        if args is None:
            self.args = ArgsProvider(
                call_from = self,
                define_params = self._params(),
                on_get_params = self._init
            )
        else:
            self.args = args
            self._init(args)

        # Accumulated errors.
        self.stats = defaultdict(lambda : Stats())

        self._cb = {}
        if mi is not None:
            self.model_interface = mi

    def set_model_interface(self, mi):
        self.model_interface = mi

    def _params(self):
        return []

    @abc.abstractmethod
    def _init(self, args):
        pass

    @abc.abstractmethod
    def update(self, batch):
        ''' Return stats to show/accumulate, and variables to backprop '''
        pass

    def add_cb(self, name, cb):
        self.cb[name] = cb

    def run(self, batch, update_params=True):
        self.model_interface.zero_grad()
        self.update(batch)
        # If update_params is False, we only compute the gradient, but not update the parameters.
        if update_params:
            self.model_interface.update_weights()

    def print_stats(self, global_counter=None, reset=True):
        for k in sorted(self.stats.keys()):
            v = self.stats[k]
            print(v.summary(info=str(global_counter) + ":" + k))
            if reset: v.reset()

# Some utility functions
def average_norm_clip(grad, clip_val):
    ''' The first dimension will be batchsize '''
    batchsize = grad.size(0)
    avg_l2_norm = 0.0
    for i in range(batchsize):
        avg_l2_norm += grad[i].data.norm()
    avg_l2_norm /= batchsize
    if avg_l2_norm > clip_val:
        # print("l2_norm: %.5f clipped to %.5f" % (avg_l2_norm, clip_val))
        grad *= clip_val / avg_l2_norm

def accumulate(acc, new):
    ret = { k: new[k] if a is None else a + new[k] for k, a in acc.items() if k in new }
    ret.update({ k : v for k, v in new.items() if not (k in acc) })
    return ret

def check_terminals(has_terminal, batch):
    # Block backpropagation if we go pass a terminal node.
    for i, terminal in enumerate(batch["terminal"]):
        if terminal: has_terminal[i] = True

def check_terminals_anyT(has_terminal, batch, T):
    for t in range(T):
        check_terminals(has_terminal, batch[t])


