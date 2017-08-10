import argparse
from datetime import datetime

import sys
import os
import random
from collections import defaultdict

from rlpytorch import load_module, SingleProcessRun, ArgsProvider

class StatsCollector:
    def __init__(self):
        self.id2seqs_actor = defaultdict(lambda : -1)
        self.idgseq2action = defaultdict(lambda : -1)
        self.id2seqs_train = defaultdict(lambda : -1)

    def set_params(self, params):
        self.params = params

    def _title(self, prompt, sel, t, i):
        return "[%s][id=%d][batchidx=%d][t=%d][seq=%d][game_counter=%d]: " % (prompt, sel["id"][t][i], i, t, sel["seq"][t][i], sel["game_counter"][t][i])

    def _debug(self, prompt):
        import pdb
        pdb.set_trace()
        raise ValueError(prompt)

    def actor(self, sel, sel_gpu):
        '''Check the states for an episode.'''
        b = sel.hist(0)
        batchsize = b["id"].size(0)
        # import pdb
        # pdb.set_trace()
        # Return random actions.
        actions = [random.randint(0, self.params["num_action"]-1) for i in range(batchsize)]

        # Check whether id is duplicated.
        ids = set()
        for i, (id, seq, game_counter, last_terminal, a) in enumerate(zip(b["id"], b["seq"], b["game_counter"], b["last_terminal"], actions)):
            # print("[%d] actor %d, seq %d" % (i, id, seq))
            prompt = self._title("actor", sel, 0, i)
            if id not in ids:
                ids.add(id)
            else:
                self._debug("%s, id duplicated!" % prompt)

            if last_terminal:
                self.id2seqs_actor[id] = -1
            predicted = self.id2seqs_actor[id] + 1
            if seq != predicted:
                self._debug("%s, should be seq = %d" % (prompt, predicted))
            self.id2seqs_actor[id] += 1

            key = (id, seq, game_counter)
            self.idgseq2action[key] = a

        return dict(a=actions)

    def train(self, sel, sel_gpu):
        T = sel["id"].size(0)
        batchsize = sel["id"].size(1)

        # Check whether the states are consecutive
        for i in range(batchsize):
            id = sel["id"][0][i]
            last_seq = self.id2seqs_train[id]
            # print("train %d, last_seq: %d" % (id, last_seq))
            for t in range(T):
                prompt = self._title("train", sel, t, i)

                if sel["last_terminal"][t][i]:
                    last_seq = -1
                if sel["seq"][t][i] != last_seq + 1:
                    self._debug("%s. Invalid next seq. seq should be %d" % (prompt, last_seq + 1))
                last_seq += 1

                # Check whether the actions remains the same.
                if t < T - 1:
                    key = (id, sel["seq"][t][i], sel["game_counter"][t][i])
                    recorded_a = self.idgseq2action[key]
                    actual_a = sel["a"][t][i]
                    if recorded_a != actual_a:
                        self._debug("%s Action was different. recorded %d, actual %d" % (prompt, recorded_a, actual_a))

            # Overlapped by 1.
            self.id2seqs_train[id] = last_seq - 1

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    game = load_module(os.environ["game"]).Loader()
    collector = StatsCollector()
    runner = SingleProcessRun()

    args_providers = [game, runner]

    all_args = ArgsProvider.Load(parser, args_providers)

    GC = game.initialize()
    # GC.setup_gpu(0)
    collector.set_params(GC.params)

    GC.reg_callback("actor", collector.actor)
    GC.reg_callback("train", collector.train)
    GC.reg_sig_int()

    runner.setup(GC)
    runner.run()
