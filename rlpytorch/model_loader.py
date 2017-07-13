# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

from .args_utils import ArgsProvider

class ModelLoader:
    def __init__(self, model_class):
        self.model_class = model_class
        self.args = ArgsProvider(
            call_from = self,
            define_params = [
                ("gpu", dict(type=str, help="gpu to use", default=None)),
                ("load", dict(type=str, help="load model", default=None))
            ]
        )

    def load_model(self, params):
        args = self.args
        args.params = params
        # Initialize models.
        model = self.model_class(args)

        if args.load is not None:
            print("Load from " + args.load)
            model.load(args.load)

        if not getattr(args, "cpu", False):
            model.cuda()

        return model
