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
        define_args =[
            ("gpu", dict(type=int, help="gpu to use", default=None)),
            ("load", dict(type=str, help="load model", default=None))
        ]
        if hasattr(model_class, "get_define_args"):
            define_args += model_class.get_define_args()

        self.args = ArgsProvider(
            call_from = self,
            define_args = define_args
        )

    def load_model(self, params):
        args = self.args
        args.params = params
        # Initialize models.
        model = self.model_class(args)

        if args.load is not None:
            print("Load from " + args.load)
            model.load(args.load)

        if args.gpu is not None and args.gpu >= 0:
            model.cuda(device_id=args.gpu)

        return model
