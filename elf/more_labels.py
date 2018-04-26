# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

from rlpytorch import ArgsProvider

class MoreLabels:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("additional_labels", dict(type=str, default=None, help="Add additional labels in the batch. E.g., id,seq,last_terminal")),
            ]
        )

    def add_labels(self, desc):
        args = self.args
        if args.additional_labels is None: return

        extra = args.additional_labels.split(",")
        for _, v in desc.items():
            v["input"]["keys"].update(extra)

