# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import abc
from collections import defaultdict
from .utils import Stats
from .args_utils import ArgsProvider

class LearningMethod:
    def __init__(self, mi=None, args=None):
        '''Initialize a learning method. ``args`` encodes the hyperparameters that the learning method reads from the command line. For example
        ::
            args = [
                ("T", 6),
                ("use_gradient_clip", dict(action="store_true"))
            ]

        means that

          * there is an argument ``T`` whose default value is ``6`` (int).
          * there is a binary switch ``use_gradient_clip``.

        When the second element of an entry in ``args`` is a dict, it will be used as the specification of ``ArgumentParser`` defined in :mod:`argparse`.
        Once the arguments are loaded, they are accessible as attributes in ``self.args``. The class also call :func:`_init` so that the derived class
        has a chance to initialize relevant variables.

        Arguments:
            mi (ModelInterface): The model to be updated using input batches.
            args (list): The arguments used by the learning method.
              If None (which is often the case), then :class:`LearningMethod`, as a base class, will automatically
              call ``_args()`` of the derived class to get the arguments.
        '''
        if args is None:
            self.args = ArgsProvider(
                call_from = self,
                define_args = self._args(),
                more_args = self._more_args(),
                on_get_args = self._init
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
        '''Set the model to be updated. '''
        self.model_interface = mi

    def _args(self):
        '''Return the arguments that the learning method will read from the command line'''
        return []

    def _more_args(self):
        '''Return the arguments claim by other part of the program that the learning method will use'''
        return []

    @abc.abstractmethod
    def _init(self, args):
        '''The function is called when the learning method gets the arguments from the command line. Derived class overrides this function.
        Arguments:
            args(ArgsProvider): The arguments that have been read from the command line.
        '''
        pass

    @abc.abstractmethod
    def update(self, batch):
        '''Compute the gradient of the model using ``batch``, and accumulate relevant statistics.
        Note that all the arguments from command line are accessible as attributes in ``self.args``.
        '''
        pass

    def add_cb(self, name, cb):
        self.cb[name] = cb

    def run(self, batch, update_params=True):
        '''The method does the following

          * Zero the gradient of the model.
          * Compute the gradient with ``batch`` and accumulate statistics (call :func:`update`).
          * If ``update_params == True``, update the parameters of the model.
        '''
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


