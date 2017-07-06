# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import sys

class ArgsProvider:
    def __init__(self, define_params=[], more_params=[], on_get_params=None, call_from=None, child_providers=[]):
        self._define_params = define_params
        self._more_params = more_params
        self._on_get_params = on_get_params
        self._arg_keys = list(list(zip(*self._define_params))[0])
        self._child_providers = child_providers
        self._call_from = call_from

    def init(self, parser):
        group_name = type(self._call_from).__name__ if self._call_from is not None else "Options"
        group = parser.add_argument_group(group_name)
        for key, options in self._define_params:
            if isinstance(options, (int, float, str)):
                group.add_argument("--" + key, type=type(options), default=options)
            else:
                group.add_argument("--" + key, **options)

        for child_provider in self._child_providers:
            child_provider.init(parser)

    def set(self, args, **kwargs):
        ''' kwargs is used to override any existing args '''
        # First check all the children.
        for child_provider in self._child_providers:
            child_provider.set(args, **kwargs)

        for key in self._arg_keys + self._more_params:
            if not hasattr(self, key):
                setattr(self, key, args.__dict__[key])

        # Override.
        for k, v in kwargs.items():
            if hasattr(self, k):
                setattr(self, k, v)

        setattr(self, "command_line", " ".join(sys.argv))
        if self._on_get_params is not None:
            self._on_get_params(args)


def args_loader(parser, args_providers, cmd_line=sys.argv[1:]):
    for provider in args_providers:
        provider.args.init(parser)

    args = parser.parse_args(cmd_line)
    print(args)

    for provider in args_providers:
        provider.args.set(args)

    return args
