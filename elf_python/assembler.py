# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

from collections import defaultdict, Counter
from .circular_queue import CQueue

class ExpOp:
    def __init__(self, excluded_entries=(), hist_entries=None, num_hist=0, use_future=True):
        # For each agent, we have the following memory layout:
        # agent: hhhhh bbbbbbbbbbbb ffff
        # h = history slot (of length num_hist),
        # b = current batch (of length T),
        # f = future slot (of length num_future)

        # History is used to fill in the history slot of batch[0].
        # all histroy entries will be prefixed "hist%d_" % (hist_idx)
        # The entries with "last_" prefix will be inserted to the previous
        # state. Therefore, we need "future" slots to fill the
        # corresponding entry in the newest experience with.

        self.excluded_entries = excluded_entries

        self.num_hist = num_hist
        self.hist_entries = hist_entries
        self.use_hist = self.hist_entries is not None and self.num_hist > 0

        self.use_future = use_future
        self.num_future = 1 if use_future else 0
        if use_future:
            self.last_symbol = "last_"
            self.len_last_symbol = len(self.last_symbol)

    def _get_hist(self, mem, j):
        # Do we want to extract more information from history?
        # Old history comes first.
        if not self.use_hist: return
        entry_hist = { }
        hist_idx = self.num_hist
        for i in range(self.num_hist):
            m = mem[i + j]
            entry_hist.update({ "hist%d_%s" % (hist_idx, k) : m[k] for k in self.hist_entries if k in m })
            hist_idx -= 1
        return entry_hist

    def _get_future(self, mem, j):
        if not self.use_future: return

        # Assume that mem[num_hist] and mem[num_hist+1] will never fail.
        m = mem[self.num_hist + j]
        m_next = mem[self.num_hist + j + 1]

        entry_from_next = { k[self.len_last_symbol:] : v for k, v in m_next.items() if k.startswith(self.last_symbol) }

        if m_next["_seq"] - m["_seq"] != 1:
            # print("next_seq = %d, curr_seq = %d" % (m_next["_seq"], curr_seq))
            # If the sequence is not continous, we clear all the last_info.
            # TODO need to remove this. It is sufficient to put a
            # terminal here, and it is not necessary to set
            # entry_from_next[k] to be zero.
            for k in entry_from_next.keys():
                entry_from_next[k] = 0.0
            # Put a terminal.
            entry_from_next["terminal"] = "incomplete"
        else:
            if "terminal" not in entry_from_next:
                entry_from_next["terminal"] = "no"
        return entry_from_next

    def extract(self, mem, T):
        entries = []
        for j in range(T):
            m = mem[self.num_hist + j]
            entry = { k : v for k, v in m.items() if k not in self.excluded_entries or k.startswith("_") }

            entry_from_next = self._get_future(mem, j)
            if entry_from_next is not None: entry.update(entry_from_next)

            # Do we want to extract more information from history?
            if j == 0:
                entry_hist = self._get_hist(mem, j)
                if entry_hist is not None: entry.update(entry_hist)

            entries.append(entry)

        return entries

    def size(self):
        return self.num_hist + self.num_future

    def hist_size(self):
        return self.num_hist

class BatchAssembler:
    def __init__(self, batchsize, exp_op, T=1):
        self.exp_op = exp_op
        self.T = T
        self.num_extra = self.exp_op.size()
        self.total_count = 0

        # Length limit for each agent's history
        self.memory_length_limit = T * 3 + self.num_extra

        self.batchsize = batchsize
        self.memory = defaultdict(lambda : CQueue(self.memory_length_limit))
        self.counts = Counter()

    def sample_count(self):
        return self.total_count

    def get_batch(self, incomplete=False):
        ''' Draw a batch of size, it will guarantee to return a batch '''
        if incomplete:
            n = self.total_count
            # print("[%s]: Sending incomplete batch: %d/%d" % (self.name, self.total_count, self.batchsize))
        else:
            # Check if we have enough data to return?
            if self.total_count < self.batchsize: return
            n = self.batchsize

        # qs has the following structure:
        # len(qs) == T
        # len(qs[t]) == n (batchsize)
        qs = [ list() for t in range(self.T)]

        # Each element of the batch comes from a different environemnt.
        counts_dup = Counter(self.counts)

        counter = 0
        for idt, count in counts_dup.items():
            mem = self.memory[idt]

            if len(mem) < count * self.T + self.num_extra:
                # XXX Something wrong happens!
                print("%s: count = %d, T = %d, len = %d" % (idt, count, self.T, len(mem)))

            for _ in range(count):
                this_mem = mem.peekn_top(self.T + self.num_extra)
                entries = self.exp_op.extract(this_mem, self.T)
                mem.popn(self.T)

                for j, entry in enumerate(entries):
                    qs[j].append(entry)

                counter += 1
                if counter >= n:
                    break

            # Remove the current idt if there is not enough examples.
            self.counts[idt] = (len(mem) - self.num_extra) // self.T
            if counter >= n:
                break

        # print("Recount: %d" % recount)
        self.total_count -= n
        return qs

    def feed(self, m, hold_batch=False, hist_fill=None):
        ''' Feed the data in. if a batch of size batchsize can be extracted,
            return the batch
        '''
        agent = m["_agent_name"]
        mem = self.memory[agent]

        if hist_fill is not None:
            while len(mem) < self.exp_op.hist_size():
                mem.push(hist_fill)

        mem.push(m)
        # Keep memory size upper-bounded.
        if len(mem) >= self.memory_length_limit:
            mem.pop()
            return

        if len(mem) >= (self.counts[agent] + 1) * self.T + self.num_extra:
            self.counts[agent] += 1
            self.total_count += 1

        # Check whether the condition satisfies.
        if not hold_batch and self.total_count >= self.batchsize:
            # print("Get one full batch, loop_count = %d" % self.loop_count)
            return self.get_batch()

    def feeds(self, ms, ordered=False, **kwargs):
        for m in ms:
            self.feed(m, hold_batch=True, **kwargs)
        batch = self.get_batch()
        if batch is None: return

        # Keep the order if possible.
        if ordered:
            agent2idx = { b["_agent_name"] : i for i, b in enumerate(batch[0]) }
            # If it is not 1:1 mapping, the following code would throw
            # exceptions.
            for t in range(len(batch)):
                batch[t] = [ batch[t][agent2idx[m["_agent_name"]]] for m in ms ]
        return batch

    def print_stats(self):
        ''' Print stats '''
        max_mem_len = 0
        min_mem_len = self.memory_length_limit + 1
        max_agent = ""
        min_agent = ""

        avg_mem_len = 0
        cnt = 0
        for agent, q in self.memory.items():
            if len(q) > max_mem_len:
                max_mem_len = len(q)
                max_agent = agent
            if len(q) < min_mem_len:
                min_mem_len = len(q)
                min_agent = agent
            avg_mem_len += len(q)
            cnt += 1

        print("Assembler: Limit: %d, Min: %d [%s], Max: %d [%s], Avg: %.2f[%d]"
              % (self.memory_length_limit, min_mem_len, min_agent,
                 max_mem_len, max_agent, avg_mem_len / cnt, cnt))

