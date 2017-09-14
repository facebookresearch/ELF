# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
import os
import sys
from .args_provider import ArgsProvider
# from .utils.utils import get_total_size

def load_module(mod):
    ''' Load a python module'''
    sys.path.insert(0, os.path.dirname(mod))
    module = __import__(os.path.basename(mod))
    return module

class ModelLoader:
    ''' Load a previously saved model'''
    def __init__(self, model_class, model_idx=None):
        ''' Initialization, specify the model to be loaded, function to call after loading and arguments.
            omit_keys is to omit certain keys when loading, e.g. model can have extra keys. Loading will fail if extra keys are not put in ``omit_keys``
        '''

        self.model_class = model_class
        self.model_idx = model_idx

        define_args =[
            ("load", dict(type=str, help="load model", default=None)),
            ("onload", dict(type=str, help="function(s) to call after loading. e.g., reset,zero_first_layer. These functions are specified in the model", default=None)),
            ("omit_keys", dict(type=str, help="omitted keys when loading.", default=None)),
        ]

        self.define_args = define_args
        if model_idx is not None:
            self.define_args_final = [ (e[0] + str(model_idx), e[1]) for e in define_args ]
        else:
            self.define_args_final = self.define_args

        combined_args = self.define_args_final + model_class.get_define_args() if hasattr(model_class, "get_define_args") else self.define_args_final

        self.args = ArgsProvider(
            call_from = self,
            define_args = combined_args,
            more_args = ["gpu"],
            on_get_args = self._on_get_args
        )

    def _on_get_args(self, _):
        '''Set all the arguments'''
        for arg, arg_final in zip(self.define_args, self.define_args_final):
            setattr(self, arg[0], getattr(self.args, arg_final[0]))

    def load_model(self, params):
        '''Actually loading the model with initialized args.
           Call onload funtions if needed.
        '''
        args = self.args
        args.params = params
        # Initialize models.
        model = self.model_class(args)

        if self.load is not None:
            print("Load from " + self.load)
            if self.omit_keys is not None:
                omit_keys = args.omit_keys.split(",")
                print("Omit_keys = " + str(omit_keys))
            else:
                omit_keys = []
            model.load(self.load, omit_keys=omit_keys)
            print("Loaded = " + model.filename)

        if self.onload is not None:
            for func in self.onload.split(","):
                try:
                    getattr(model, func)()
                    print("Called %s for model" % func)
                except:
                    print("calling func = %s failed!" % func)
                    sys.exit(1)

        if self.onload is not None:
            for func in self.onload.split(","):
                try:
                    getattr(model, func)()
                    print("Called %s for model" % func)
                except:
                    print("calling func = %s failed!" % func)
                    sys.exit(1)

        if args.gpu is not None and args.gpu >= 0:
            model.cuda(device_id=args.gpu)

        return model

def load_env(envs, num_models=None):
    ''' Load envs. envs will be specified as environment variables, more specifically, ``game``, ``model_file`` and ``model`` are required.

    Returns:
        ``game`` : game module
        `` method``: Learning method used
        ``model_loaders``: loaders for model
    '''
    game = load_module(envs["game"]).Loader()
    model_file = load_module(envs["model_file"])
    model_class, method_class = model_file.Models[envs["model"]]
    method = method_class()

    # You might want multiple models loaded.
    if num_models is None:
        model_loaders = [ ModelLoader(model_class) ]
    else:
        model_loaders = [ ModelLoader(model_class, model_idx=i) for i in range(num_models) ]

    return game, method, model_loaders
