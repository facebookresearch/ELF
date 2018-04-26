# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

# Console for DarkForest
import sys
import os
from collections import Counter
from rlpytorch import load_env, Evaluator, ModelInterface, ArgsProvider, EvalIters

def move2xy(v):
    x = ord(v[0].lower()) - ord('a')
    # Skip 'i'
    if x >= 9: x -= 1
    y = int(v[1:]) - 1
    return x, y

def move2action(v):
    x, y = move2xy(v)
    return x * 19 + y

def xy2move(x, y):
    if x >= 8: x += 1
    return chr(x + 65) + str(y + 1)

def action2move(a):
    x = a // 19
    y = a % 19
    return xy2move(x, y)

def plot_plane(v):
    s = ""
    for j in range(v.size(1)):
        for i in range(v.size(0)):
            if v[i, v.size(1) - 1 - j] != 0:
                s += "o "
            else:
                s += ". "
        s += "\n"
    print(s)


def topk_accuracy2(batch, state_curr, topk=(1,)):
    pi = state_curr["pi"]
    import torch
    if isinstance(pi, torch.autograd.Variable):
        pi = pi.data
    score, indices = pi.sort(dim=1, descending=True)

    maxk = max(topk)
    topn_count = [0] * maxk

    for ind, gt in zip(indices, batch["offline_a"][0]):
        for i in range(maxk):
            if ind[i] == gt[0]:
                topn_count[i] += 1

    for i in range(maxk):
        topn_count[i] /= indices.size(0)

    return [ topn_count[i - 1] for i in topk ]


class DFConsole:
    def __init__(self):
        self.exit = False

    def check(self, batch):
        reply = self.evaluator.actor(batch)
        topk = topk_accuracy2(batch, reply, topk=(1,2,3,4,5))
        for i, v in enumerate(topk):
            self.check_stats[i] += v
        if sum(topk) == 0: self.check_stats[-1] += 1

    def actor(self, batch):
        reply = self.evaluator.actor(batch)
        return reply

    def prompt(self, prompt_str, batch):
        if self.last_move_idx is not None:
            curr_move_idx = batch["move_idx"][0][0]
            if curr_move_idx - self.last_move_idx == 1:
                self.check(batch)
                self.last_move_idx = curr_move_idx
                return
            else:
                n = sum(self.check_stats.values())
                print("#Move: " + str(n))
                accu = 0
                for i in range(5):
                    accu += self.check_stats[i]
                    print("Top %d: %.3f" % (i, accu / n))
                self.last_move_idx = None

        print(batch.GC.ShowBoard(0))
        # Ask user to choose
        while True:
            if getattr(self, "repeat", 0) > 0:
                self.repeat -= 1
                cmd = self.repeat_cmd
            else:
                cmd = input(prompt_str)
            items = cmd.split()
            if len(items) < 1:
                print("Invalid input")

            reply = dict(pi=None, a=None)

            try:
                if items[0] == 'p':
                    reply["a"] = move2action(items[1])
                    return reply
                elif items[0] == 'c':
                    return self.evaluator.actor(batch)
                elif items[0] == "s":
                    channel_id = int(items[1])
                    plot_plane(batch["s"][0][0][channel_id])
                elif items[0] == "u":
                    batch.GC.UndoMove(0)
                    print(batch.GC.ShowBoard(0))
                elif items[0] == "h":
                    handicap = int(items[1])
                    batch.GC.ApplyHandicap(0, handicap)
                    print(batch.GC.ShowBoard(0))
                elif items[0] == "a":
                    reply = self.evaluator.actor(batch)
                    if "pi" in reply:
                        score, indices = reply["pi"].squeeze().sort(dim=0, descending=True)
                        first_n = int(items[1])
                        for i in range(first_n):
                            print("%s: %.3f" % (action2move(indices[i]), score[i]))
                    else:
                        print("No key \"pi\"")
                elif items[0] == "check":
                    print("Top %d" % self.check(batch))

                elif items[0] == 'check2end':
                    self.check_stats = Counter()
                    self.check(batch)
                    self.last_move_idx = batch["move_idx"][0][0]
                    if len(items) == 2:
                        self.repeat = int(items[1])
                        self.repeat_cmd = "check2end_cont"
                    return

                elif items[0] == "check2end_cont":
                    if not hasattr(self, "check_stats"):
                        self.check_stats = Counter()
                    self.check(batch)
                    self.last_move_idx = batch["move_idx"][0][0]
                    return

                elif items[0] == "aug":
                    print(batch["aug_code"][0][0])
                elif items[0] == "show":
                    print(batch.GC.ShowBoard(0))
                elif items[0] == "dbg":
                    import pdb
                    pdb.set_trace()
                elif items[0] == 'offline_a':
                    if "offline_a" in batch:
                        for i, offline_a in enumerate(batch["offline_a"][0][0]):
                            print("[%d]: %s" % (i, action2move(offline_a)))
                    else:
                        print("No offline_a available!")
                elif items[0] == "exit":
                    self.exit = True
                    return reply
                else:
                    print("Invalid input: " + cmd + ". Please try again")
            except Exception as e:
                print("Something wrong! " + str(e))

    def main_loop(self):
        evaluator = Evaluator(stats=False)
        # Set game to online model.
        env, args = load_env(os.environ, evaluator=evaluator, overrides=dict(num_games=1, batchsize=1, num_games_per_thread=1, greedy=True, T=1, additional_labels="aug_code,move_idx"))

        GC = env["game"].initialize()
        model = env["model_loaders"][0].load_model(GC.params)
        mi = ModelInterface()
        mi.add_model("model", model)
        mi.add_model("actor", model, copy=True, cuda=args.gpu is not None, gpu_id=args.gpu)
        mi["model"].eval()
        mi["actor"].eval()

        self.evaluator = evaluator
        self.last_move_idx = None

        def human_actor(batch):
            print("In human_actor")
            return self.prompt("DF> ", batch)

        def actor(batch):
            return self.actor(batch)

        def train(batch):
            self.prompt("DF Train> ", batch)

        evaluator.setup(sampler=env["sampler"], mi=mi)

        GC.reg_callback_if_exists("actor", actor)
        GC.reg_callback_if_exists("human_actor", human_actor)
        GC.reg_callback_if_exists("train", train)

        GC.Start()

        evaluator.episode_start(0)

        while True:
            GC.Run()
            if self.exit: break
        GC.Stop()

if __name__ == '__main__':
    console = DFConsole()
    console.main_loop()
