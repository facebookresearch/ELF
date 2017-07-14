# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import sys

class ArgsProvider:
    def __init__(self, define_args=[], more_args=[], on_get_args=None, call_from=None, child_providers=[]):
        '''Define arguments to be loaded from the command line. Example usage
        ::
            args = ArgsProvider(
                define_args = [
                    ("T", 6),
                    ("update_gradient", dict(action="store_true"))
                ],
                more_args = ["batchsize", "num_games"],
                on_get_args = lambda args: print(args.T)
            )

        Parameters:
            define_args(list of tuple): a list of arguments to be read from the command line. For each tuple in
              the list, its first element is the argument name, while the second element is either a ``int/str/float``
              which is the default value of the argument, or a dict that contains the specification sent to
              :class:`ArgumentParser`.
            more_args(list): a list of argument names that are specified by other instance of :class:`ArgsProvider`
              but will be used by this instance. For example, ``num_games`` and ``batchsize`` is often used by multiple classes.
            on_get_args(lambda function that takes an argument of type :class:`ArgsProvider`): a callback function that will
              be invoked after the arguments are read from the command line.
            call_from(parent class instance): The parent class instance that calls :func:`__init__``. :class:`ArgsProvider` uses
              the class name of ``call_from`` as the group name of the arguments specified by ``define_args``.
            child_providers(list of :class:`ArgsProvider`): The current class instance also collects the arguments required by the instances from ``child_providers``.
        '''

        self._define_args = define_args
        self._more_args = more_args
        self._on_get_args = on_get_args
        self._arg_keys = list(list(zip(*self._define_args))[0])
        self._child_providers = child_providers
        self._call_from = call_from

    def init(self, parser):
        group_name = type(self._call_from).__name__ if self._call_from is not None else "Options"
        group = parser.add_argument_group(group_name)
        for key, options in self._define_args:
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

        for key in self._arg_keys + self._more_args:
            if not hasattr(self, key):
                setattr(self, key, args.__dict__[key])

        # Override.
        for k, v in kwargs.items():
            if hasattr(self, k):
                setattr(self, k, v)

        setattr(self, "command_line", " ".join(sys.argv))
        if self._on_get_args is not None:
            self._on_get_args(args)

    def Load(parser, args_providers, cmd_line=sys.argv[1:]):
        '''Load args from ``cmd_line``

        Parameters:
            parser(ArgumentParser): The argument parser.
            args_providers(list of class instances): List of class instances. Each item has an attribute ``args``. These ``args`` will add arguments to ``parser``.
            cmd_line(list of str, or str): command line used to load the arguments. If not specified, use ``sys.argv``.

        Returns:
            A class instance whose attributes contain all loaded arguments.
        '''
        for provider in args_providers:
            provider.args.init(parser)

        args = parser.parse_args(cmd_line)
        print(args)

        for provider in args_providers:
            provider.args.set(args)

        return args
