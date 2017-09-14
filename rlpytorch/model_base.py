# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import torch
import torch.nn as nn
from copy import deepcopy
from torch.autograd import Variable
from time import sleep
from collections import OrderedDict

class Model(nn.Module):
    ''' Base class for an RL model, it is a wrapper for ``nn.Module``'''

    def __init__(self, args):
        ''' Initialize model with args'''
        super(Model, self).__init__()
        self.step = 0
        self.args = deepcopy(args)
        self.volatile = False

    def clone(self, gpu=None):
        '''

        Returns:
            Deep copy an existing model
        '''
        model = type(self)(self.args)
        model.load_state_dict(deepcopy(self.state_dict()))
        model.step = self.step
        if gpu is not None:
            model.cuda(gpu)
        return model

    def set_volatile(self, volatile):
        ''' Set model to ``volatile``'''
        self.volatile = volatile

    def _var(self, x):
        ''' Convert x to a pytorch Variable.

        Returns:
            Variable for x
        '''
        return Variable(x, volatile=self.volatile)

    def before_update(self):
        ''' Customized operations for each model before update. To be extended. '''
        pass

    def save(self, filename, num_trial=10):
        ''' Save current model, step and args to ``filename``'''
        stats = self.clone().cpu().state_dict()
        # Note that the save might experience issues, so if we encounter errors,
        # try a few times and then give up.
        content = dict(stats_dict=stats, step=self.step, args=self.args)
        for i in range(num_trial):
            try:
                torch.save(content, filename)
                return
            except:
                sleep(1)
        print("Failed to save %s after %d trials, giving up ..." % (filename, num_trial))

    def load(self, filename, omit_keys=[]):
        ''' Load current model, step and args from ``filename``'''
        data = torch.load(filename)
        if "args" in data:
            # Reload the structure of the model.
            self._init(data["args"])
            self.args = deepcopy(data["args"])

        if isinstance(data, OrderedDict):
            self.load_state_dict(data)
        else:
            for k in omit_keys:
                del data["stats_dict"][k + ".weight"]
                del data["stats_dict"][k + ".bias"]

            self.load_state_dict(data["stats_dict"])
        self.step = data.get("step", 0)
        self.filename = data.get("filename", filename)

    def load_from(self, model):
        ''' Load from an existing model. State is deep copied '''
        if hasattr(model, "args"):
            self.args = deepcopy(model.args)

        self.load_state_dict(model.state_dict())
        self.step = model.step

    def _init(self, args):
        ''' customized operations for each model at init. To be extended. '''
        pass

    def inc_step(self):
        ''' increment the step '''
        self.step += 1

    def signature(self):
        '''

        Returns:
            the model's signature, using step.
        '''
        return "Model[%d]" % self.step
