# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
import argparse
from .args_provider import ArgsProvider
from .sampler import Sampler
from .model_interface import ModelInterface
# from .utils.utils import get_total_size

def load_module(mod):
    ''' Load a python module'''
    sys.path.insert(0, os.path.dirname(mod))
    module = __import__(os.path.basename(mod))
    return module

class ModelLoader:
    ''' Class to load a previously saved model'''
    def __init__(self, model_class, model_idx=None):
        ''' Initialization, Accepted arguments:
        ``load``:specify the model to be loaded
        ``onload``: function to call after loading and arguments.
        ``omit_keys``: to omit certain keys when loading, e.g. model can have extra keys if trained with additional tasks.
        Loading will fail if extra keys are not put in ``omit_keys``
        Args:
            model_class(class): class name of the model
            model_idx(int): index of the model to be loaded. There may be multiple model in an `ModelInterface` to load.
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

        Args:
            params(dict): additinoal parameters to be put into args.
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

        if args.gpu is not None and args.gpu >= 0:
            model.cuda(device=args.gpu)

        return model

def load_env(envs, num_models=None, overrides=dict(), defaults=dict(), **kwargs):
    ''' Load envs. envs will be specified as environment variables, more specifically, ``game``, ``model_file`` and ``model`` are required.

    Returns:
        env: dict of
            ``game`` : game module
            ``method``: Learning method used
            ``model_loaders``: loaders for model
        all_args: loaded arguments from `ArgsPrivider`
    '''
    game = load_module(envs["game"]).Loader()
    model_file = load_module(envs["model_file"])
    # TODO This is not good, need to fix.
    if len(model_file.Models[envs["model"]]) == 2:
        model_class, method_class = model_file.Models[envs["model"]]
        sampler_class = Sampler
    else:
        model_class, method_class, sampler_class = model_file.Models[envs["model"]]

    defaults.update(getattr(model_file, "Defaults", dict()))
    overrides.update(getattr(model_file, "Overrides", dict()))

    method = method_class()
    sampler = sampler_class()
    mi = ModelInterface()

    # You might want multiple models loaded.
    if num_models is None:
        model_loaders = [ ModelLoader(model_class) ]
    else:
        model_loaders = [ ModelLoader(model_class, model_idx=i) for i in range(num_models) ]

    env = dict(game=game, method=method, sampler=sampler, model_loaders=model_loaders, mi=mi)
    env.update(kwargs)

    parser = argparse.ArgumentParser()
    all_args = ArgsProvider.Load(parser, env, global_defaults=defaults, global_overrides=overrides)
    return  env, all_args
