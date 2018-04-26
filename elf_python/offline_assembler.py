# 1717right (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

from collections import defaultdict
import random

class OfflineReplay:
    ''' Offline Replay '''
    def __init__(self, priority_level=1):
        self.replays = [list() for i in range(priority_level)]

    def add(self, m, level=0):
        self.replays[level].append(m)

    def clear(self):
        ''' Clear everything '''
        self.replays = [list() for i in range(len(self.replays))]

    def sample(self, T=1):
        ''' Sample '''
        level = random.randint(0, len(self.replays) - 1)
        start = random.randint(0, len(self.replays[level]) - T)

        # Return an iterator.
        return (m for m in self.replays[level][start:start+T])

class BatchAssemblerOffline:
    def __init__(self, batchsize, exp_op, T=1, data_switch=None):
        self.T = T
        self.exp_op = exp_op
        self.num_extra = self.exp_op.size()
        self.batchsize = batchsize
        self.memory = defaultdict(lambda : OfflineReplay())

        self.num_total_collections = 100000
        self.freq_prompt_collection = self.num_total_collections / 10

        self.data_switch = data_switch
        self.clear()

    def feed(self, m, hist_fill={}):
        agent = m["_agent_name"]
        mem = self.memory[agent]

        #
        #if hist_fill is not None:
        #    while len(mem) < self.exp_op.hist_size():
        #        mem.add(hist_fill)
        mem.add(m)
        self.total_count += 1
        if self.total_count % self.freq_prompt_collection == 0:
            print("[Offline Collecting]: %d" % self.total_count)
        if self.total_count >= self.num_total_collections and self.data_switch is not None:
            if self.data_switch.get():
                print("[Offline collection done]")
                self.data_switch.set(False)

    def sample_count(self):
        return self.total_count

    def get_batch(self, incomplete=False):
        ''' Random sample agent_names '''
        # If data flow in now, then we skip.
        if self.data_switch is not None and self.data_switch.get(): return

        if self.available_agent_names is None:
            print("[Start to get_batch]")
            self.available_agent_names = list(self.memory.keys())

        qs = [ list() for t in range(self.T)]
        for i in range(self.batchsize):
            agent_name = random.choice(self.available_agent_names)
            this_mem = list(self.memory[agent_name].sample(self.T + self.num_extra))
            entries = self.exp_op.extract(this_mem, self.T)

            for j, entry in enumerate(entries):
                qs[j].append(entry)

        self.num_get_batch += 1
        if self.num_get_batch >= 3000:
            print("[Offline collection restart]")
            self.clear()
            self.data_switch.set(True)

        return qs

    def clear(self):
        self.total_count = 0
        self.num_get_batch = 0
        self.available_agent_names = None

    def print_stats(self):
        pass


