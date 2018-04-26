# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

from ..args_provider import ArgsProvider
from collections import defaultdict, deque, Counter
from datetime import datetime
import os

class SymLink:
    def __init__(self, sym_prefix, latest_k=5):
        self.sym_prefix = sym_prefix
        self.latest_k = latest_k
        self.latest_files = deque()

    def feed(self, filename):
        self.latest_files.appendleft(filename)
        if len(self.latest_files) > self.latest_k:
            self.latest_files.pop()

        for k, name in enumerate(self.latest_files):
            symlink_file = self.sym_prefix + str(k)
            try:
                if os.path.exists(symlink_file):
                    os.unlink(symlink_file)
                os.symlink(name, symlink_file)
            except:
                print("Build symlink %s for %s failed, skipped" % (symlink_file, name))


class ModelSaver:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("record_dir", "./record"),
                ("save_prefix", "save"),
                ("save_dir", dict(type=str, default=os.environ.get("save", "./"))),
                ("latest_symlink", "latest"),
            ],
            more_args = ["num_games", "batchsize"],
            on_get_args = self._on_get_args,
        )

    def _on_get_args(self, _):
        args = self.args
        args.save = (args.num_games == args.batchsize)
        if args.save and not os.path.exists(args.record_dir):
            os.mkdir(args.record_dir)

        self.symlinker = SymLink(os.path.join(args.save_dir, args.latest_symlink))

    def feed(self, model):
        args = self.args
        basename = args.save_prefix + "-%d.bin" % model.step
        print("Save to " + args.save_dir)
        filename = os.path.join(args.save_dir, basename)
        print("Filename = " + filename)
        model.save(filename)
        # Create a symlink
        self.symlinker.feed(basename)


class ValueStats:
    def __init__(self, name=None):
        self.name = name
        self.reset()

    def feed(self, v):
        self.summation += v
        if v > self.max_value:
            self.max_value = v
            self.max_idx = self.counter
        if v < self.min_value:
            self.min_value = v
            self.min_idx = self.counter

        self.counter += 1

    def summary(self, info=None):
        info = "" if info is None else info
        name = "" if self.name is None else self.name
        if self.counter > 0:
            try:
                return "%s%s[%d]: avg: %.5f, min: %.5f[%d], max: %.5f[%d]" \
                        % (info, name, self.counter, self.summation / self.counter, self.min_value, self.min_idx, self.max_value, self.max_idx)
            except:
                return "%s%s[Err]:" % (info, name)
        else:
            return "%s%s[0]" % (info, name)

    def reset(self):
        self.counter = 0
        self.summation = 0.0
        self.max_value = -1e38
        self.min_value = 1e38
        self.max_idx = None
        self.min_idx = None


def topk_accuracy(output, target, topk=(1,)):
    """Computes the precision@k for the specified values of k"""
    maxk = max(topk)
    batch_size = target.size(0)

    _, pred = output.topk(maxk, 1, True, True)
    pred = pred.t()
    correct = pred.eq(target.view(1, -1).expand_as(pred))

    res = []
    for k in topk:
        correct_k = correct[:k].view(-1).float().sum(0)
        res.append(correct_k.mul_(100.0 / batch_size))
    return res

class MultiCounter:
    def __init__(self, verbose=False):
        self.last_time = None
        self.verbose = verbose
        self.counts = Counter()
        self.stats = defaultdict(lambda : ValueStats())
        self.total_count = 0

    def inc(self, key):
        if self.verbose: print("[MultiCounter]: %s" % key)
        self.counts[key] += 1
        self.total_count += 1

    def summary(self, global_counter=None, reset=True):
        this_time = datetime.now()
        if self.last_time is not None:
            print("[%d] Time spent = %f ms" % (global_counter, (this_time - self.last_time).total_seconds() * 1000))
        self.last_time = this_time

        for key, count in self.counts.items():
            print("%s: %d/%d" % (key, count, self.total_count))

        for k in sorted(self.stats.keys()):
            v = self.stats[k]
            print(v.summary(info=str(global_counter) + ":" + k))
            if reset: v.reset()

        if reset:
            self.counts = Counter()
            self.total_count = 0


