# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
from datetime import datetime
from collections import deque, defaultdict
from torch.autograd import Variable

import sys
import os

from rlpytorch import *
from rlpytorch.stats import Stats
from rlpytorch.utils import HistState

def tensor2str(t):
    return ",".join(["%.6f" % ele for ele in t])

def merge(t, templ=None):
    output = defaultdict(lambda : list())
    for item in t:
        for k, v in item.items():
            output[k].append(v)

    output2 = dict()
    for k, v in output.items():
        if isinstance(v[0], (float, int)):
            vv = templ.clone().resize_(len(v))
            for i, entry in enumerate(v):
                vv[i] = entry
        else:
            vv = v[0].clone().resize_(len(v), *list(v[0].size()))
            for i, entry in enumerate(v):
                vv[i, :] = entry
        output2[k] = vv

    return output2

class ForwardActor:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("delay_T", 5),
                ("use_delayed_state", dict(action="store_true")),
            ],
            on_get_args = self._on_get_args
        )

        self.t0 = 0

    def _on_get_args(self, _):
        self.hs = HistState(self.args.delay_T + 1)

    def set(self, mi, sampler):
        self.mi = mi
        self.sampler = sampler

    def actor_use_delayed(self, batch):
        mi = self.mi
        T = self.args.delay_T

        # actor model.
        state_curr = mi["actor"](batch.hist(0))
        actions = self.sampler.sample(state_curr)["a"]
        batchsize = state_curr["h"].size(0)
        d = state_curr["h"].size(1)

        # Then given the previous state, perform a few forwarding.

        ids = batch["id"][0]
        Vs = state_curr["V"].data
        seqs = batch["seq"][0]

        entry = [ dict(a=a, V=V) for a, V in zip(actions, Vs) ]

        self.hs.preprocess(ids, seqs)
        self.hs.feed(ids, entry)
        old_entries = self.hs.oldest(ids, self.t0)
        reply_msg = merge(old_entries, templ=Vs[0])

        eval_iters.stats.feed_batch(batch)
        # reply_msg["rv"] = mi["actor"].step
        reply_msg["pi"] = None
        return reply_msg


    def actor(self, batch):
        mi = self.mi
        T = self.args.delay_T

        # actor model.
        state_curr = mi["actor"](batch.hist(0))
        batchsize = state_curr["h"].size(0)
        d = state_curr["h"].size(1)

        # Then given the previous state, perform a few forwarding.
        # state_hs = state_curr["h"].data[0].clone().resize_(batchsize, d)

        ids = batch["id"][0]
        hs = state_curr["h"].data
        seqs = batch["seq"][0]

        self.hs.preprocess(ids, seqs)
        self.hs.feed(ids, hs)
        state_hs = self.hs.oldest(ids, self.t0)

        # forward..
        state_curr_given_h = mi["actor"].decision(Variable(state_hs))
        # save the current state.
        action = self.sampler.sample(state_curr_given_h)["a"]
        # action = self.sampler.sample(state_curr)

        # move things forward.
        self.hs.map(ids, lambda hs: mi["actor"].transition(Variable(hs), action)["hf"].data)
        eval_iters.stats.feed_batch(batch)
        reply_msg = dict(pi=state_curr_given_h["pi"].data, a=action, V=state_curr_given_h["V"].data)
        # rv=mi["actor"].step

        if batchsize == 1:
            print("================")
            print("[%d] Predict with forward model: " % seqs[0])
            print(tensor2str(state_curr_given_h["pi"].data[0]))
            print("action = " + str(action[0]))
            print("[%d] Predict using current observation" % seqs[0])
            print(tensor2str(state_curr["pi"].data[0]))
            print("================")

        return reply_msg


if __name__ == '__main__':
    eval_iters = EvalIters()
    forward_actor = ForwardActor()
    env, args = load_env(os.environ, overrides=dict(actor_only=True), eval_iters=eval_iters, forward_actor=forward_actor)

    GC = env["game"].initialize()

    model = env["model_loaders"][0].load_model(GC.params)
    mi = env["mi"]
    mi.add_model("model", model)
    mi.add_model("actor", model, copy=True, cuda=True, gpu_id=args.gpu)

    forward_actor.set(mi, env["sampler"])
    if args.use_delayed_state:
        GC.reg_callback("actor", forward_actor.actor_use_delayed)
    else:
        GC.reg_callback("actor", forward_actor.actor)
    GC.Start()

    for _ in eval_iters.iters():
        GC.Run()
    GC.Stop()

