# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import sys
import os
from copy import deepcopy

def recursive_map(x, f):
    ''' Act a function ``f`` on ``x``. Recursively act on its items if ``x`` is a dict or list'''
    if isinstance(x, dict):
        return { k : recursive_map(v, f) for k, v in x.items() }
    elif isinstance(x, list):
        return [ recursive_map(v, f) for v in x ]
    else:
        return f(x)

class Args:
    def __init__(self, args_parsed, args_options):
        for key, value in args_parsed.__dict__.items():
            setattr(self, key, value)

        self._args_options = args_options

    def replace(self, keys):
        '''
        keys = [ (old_key1, new_key1), (old_key2, new_key2), ... ]

        Returns:
            Replaced list of keys
        '''
        res = deepcopy(self)
        for old_key, new_key in keys:
            if hasattr(res, old_key):
                setattr(res, new_key, getattr(res, old_key))
                delattr(res, old_key)
        return res

    def add_cmdline(self):
        ''' set ``command_line`` attribute, joined by ``sys.argv``'''
        setattr(self, "command_line", " ".join(sys.argv))

    def print_info(self):
        ''' Print args '''
        def stringify(x):
            if isinstance(x, str): return "\"" + x + "\""
            else: return str(x)

        print("========== Args ============")
        for group_name, options in self._args_options:
            print(group_name + ": " + ",".join(["%s=%s" % (k, stringify(self[k])) for k, v in options]))

        print("========== End of Args ============")

    def __getitem__(self, key):
        return getattr(self, key)

    def __setitem__(self, key, value):
        setattr(self, key, value)

    def __contains__(self, key):
        return hasattr(self, key)


class ArgsProvider:
    def __init__(self, define_args=[], more_args=[], on_get_args=None, call_from=None, child_providers=[], child_transforms=None):
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
            child_transforms(list of functions): Transform parent parameters to child parameters.
            global_defaults(dict of key and their default values): change default values of some parameters (the parameters can be defined elsewhere). If there is a collision, the program will err.
            global_overrides(dict of key and their overridden values): change the value of some parameters (the parameters can be defined elsewhere). If there is a collison, the program will err.
        '''

        def make_regular(options):
            if isinstance(options, (int, float, str)):
                return dict(type=type(options), default=options)
            else:
                return options

        self._define_args = [ (k, make_regular(v)) for k, v in define_args ]
        self._more_args = more_args
        self._on_get_args = on_get_args
        if len(self._define_args) == 0:
            self._arg_keys = []
        else:
            self._arg_keys = list(list(zip(*self._define_args))[0])
        self._child_providers = child_providers
        self._child_transforms = child_transforms if child_transforms is not None else [None] * len(child_providers)
        self._call_from = call_from

    def get_define_keys(self):
        ''' return all keys in _define_args in a list '''
        return [ k for k, _ in self._define_args ]


    # The following are called in ArgsPrivider.Load:
    def _collect(self, args_list):
        group_name = type(self._call_from).__name__ if self._call_from is not None else "Options"
        args_list += [ (group_name, self._define_args) ]

        for child_provider in self._child_providers:
            child_provider._collect(args_list)

    def _set(self, args):
        # First check all the children.
        for child_provider, child_transform in zip(self._child_providers, self._child_transforms):
            if child_transform is not None:
                child_args = child_transform(args)
            else:
                child_args = args
            child_provider._set(child_args)

        # Then check self.
        for key in self._arg_keys + self._more_args:
            if not hasattr(self, key):
                if key in args:
                    setattr(self, key, args[key])
                else:
                    prefix = "env_"
                    if key.startswith(prefix) and key[len(prefix):] in os.environ:
                        setattr(self, key, os.environ[key[len(prefix):]])
                    else:
                        print("Warning: key = %s cannot be found from either args or environment!" % key)

        # Finally set commandline.
        if "command_line" in args:
            setattr(self, "command_line", args["command_line"])

        if self._on_get_args is not None:
            self._on_get_args(args)

    @staticmethod
    def _GetProvider(x):
        if isinstance(x, ArgsProvider):
            return x
        elif isinstance(x.args, ArgsProvider):
            return x.args
        else:
            raise ValueError("ArgsProvider not found! " + str(x))

    @staticmethod
    def _SendArgsToParser(parser, all_args):
        for group_name, define_args in all_args:
            group = parser.add_argument_group(group_name)
            for key, options in define_args:
                try:
                    group.add_argument("--" + key, **options)
                except:
                    # If there is issues with argument name. just plot a warning.
                    print("Warning: argument %s/%s cannot be added. Skipped." % (group_name, key))

    @staticmethod
    def _ApplyDefaults(global_defaults, args_list):
        for _, define_args in args_list:
            for k, v in define_args:
                if k in global_defaults:
                    v["default"] = global_defaults[k]

    @staticmethod
    def _ApplyOverrides(global_overrides, args):
        for k, v in global_overrides.items():
            if k in args:
                args[k] = v

    @staticmethod
    def Load(parser, args_providers, cmd_line=sys.argv[1:], global_defaults=dict(), global_overrides=dict()):
        '''Load args from ``cmd_line``

        Parameters:
            parser(ArgumentParser): The argument parser.
            args_providers(list of class instances): List of class instances. Each item has an attribute ``args``. These ``args`` will add arguments to ``parser``.
            cmd_line(list of str, or str): command line used to load the arguments. If not specified, use ``sys.argv``.

        Returns:
            A class instance whose attributes contain all loaded arguments.
        '''
        args_providers = recursive_map(args_providers, ArgsProvider._GetProvider)

        args_list = []
        recursive_map(args_providers, lambda provider : provider._collect(args_list))
        ArgsProvider._ApplyDefaults(global_defaults, args_list)

        ArgsProvider._SendArgsToParser(parser, args_list)
        args = Args(parser.parse_args(cmd_line), args_list)
        args.add_cmdline()

        ArgsProvider._ApplyOverrides(global_overrides, args)

        args.print_info()
        recursive_map(args_providers, lambda provider : provider._set(args))
        return args
