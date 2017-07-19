from queue import Queue, Full, Empty
import threading
import numpy as np
from time import sleep
from collections import deque, Counter, defaultdict, OrderedDict

from .utils import *

__all__ = ["MemoryReceiver"]

def _initialize_batch_entry(batchsize, v, use_cuda=True):
    if isinstance(v, np.ndarray):
        shape = v.shape
        if v.dtype == 'int32' or v.dtype == 'int64':
            entry = torch.LongTensor(batchsize, *shape)
        else:
            # entry = np.zeros((batchsize, ) + shape, dtype=v.dtype)
            entry = torch.FloatTensor(batchsize, *shape)
    elif isinstance(v, torch.FloatTensor):
        shape = v.size()
        entry = torch.FloatTensor(batchsize, *shape)
    elif isinstance(v, list):
        entry = np.zeros((batchsize, len(v)), dtype=type(v[0]))
    elif isinstance(v, float):
        entry = torch.FloatTensor(batchsize)
    elif isinstance(v, int):
        entry = torch.LongTensor(batchsize)
    elif isinstance(v, str) or isinstance(v, bytes):
        entry = [None] * batchsize
    else:
        entry = np.zeros((batchsize), dtype=type(v))

    # Make it pinned memory
    if use_cuda and (isinstance(entry, torch.FloatTensor) or isinstance(entry, torch.LongTensor)):
        entry = entry.pin_memory()

    return entry

def _initialize_batch_cpu(batch_cpu, batch_gpu, k, v, batchsize, use_cuda=True):
    if k not in batch_cpu:
        entry = _initialize_batch_entry(batchsize, v, use_cuda=use_cuda)
        batch_cpu[k] = entry
    else:
        entry = batch_cpu[k]

    if isinstance(entry, np.ndarray):
        shape = entry.shape
    elif isinstance(entry, list):
        shape = (len(entry),)
    else:
        shape = entry.size()

    if shape[0] < batchsize:
        # Batch size becomes larger, re-initialize.
        entry = _initialize_batch_entry(batchsize, v, use_cuda=use_cuda)
        batch_cpu[k] = entry
        if k in batch_gpu: del batch_gpu[k]

    return entry, shape

def _cpu2gpu(batch_cpu, batch_gpu, allow_incomplete_batch=False):
    for batch_cpu_t, batch_gpu_t in zip(batch_cpu, batch_gpu):
        batchsize = batch_cpu_t["_batchsize"]
        batch_gpu_t["_batchsize"] = batchsize
        for k in batch_cpu_t.keys():
            if isinstance(batch_cpu_t[k], (torch.FloatTensor, torch.LongTensor)):
                if allow_incomplete_batch:
                    if len(batch_cpu_t[k].size()) == 1:
                        batch_gpu_t[k] = batch_cpu_t[k][:batchsize].cuda(async=True)
                    else:
                        batch_gpu_t[k] = batch_cpu_t[k][:batchsize, :].cuda(async=True)
                else:
                    if isinstance(batch_cpu_t[k], torch.FloatTensor):
                        if k not in batch_gpu_t:
                            batch_gpu_t[k] = torch.cuda.FloatTensor(batch_cpu_t[k].size())
                        batch_gpu_t[k].copy_(batch_cpu_t[k], async=True)

                    elif isinstance(batch_cpu_t[k], torch.LongTensor):
                        if k not in batch_gpu_t:
                            batch_gpu_t[k] = torch.cuda.LongTensor(batch_cpu_t[k].size())
                        batch_gpu_t[k].copy_(batch_cpu_t[k], async=True)
            else:
                batch_gpu_t[k] = batch_cpu_t[k]

def _make_batch(batch, q, use_cuda=True, allow_incomplete_batch=False):
    ''' Lots of hacks in this function, need to fix in the future.'''
    if "cpu" not in batch:
        batch.update({ "cpu" : [], "gpu" : [] })

    # For input q:
    # len(q) == T
    # len(q[t]) == batchsize
    # q[t][batch_id] is a dict, e.g., q[t][batch_id] = { "s" : np.array, "a" : int, "r" : float }

    # For output:
    # batch_cpu is a list of dict, e.g., batch_cpu[t] = { "s" : FloatTensor(batchsize, channel, w, h), "a" : FloatTensor(batchsize) }

    T = len(q)
    batchsize = len(q[0])

    # Time span of the batch.
    if len(batch["cpu"]) != T:
        batch["cpu"] = [dict() for i in range(T)]
        batch["gpu"] = [dict() for i in range(T)]

    batch_cpu = batch["cpu"]
    batch_gpu = batch["gpu"]

    for q_t, batch_cpu_t, batch_gpu_t in zip(q, batch_cpu, batch_gpu):
        batch_cpu_t["_batchsize"] = batchsize
        for k, v in q_t[0].items():
            entry, shape = _initialize_batch_cpu(batch_cpu_t, batch_gpu_t, k, v, batchsize, use_cuda=use_cuda)
            if len(shape) == 1:
                for i in range(batchsize):
                    entry[i] = q_t[i][k]
            else:
                # TODO: Remove this once np.array to torch assignment has been implemented.
                if isinstance(q_t[0][k], np.ndarray):
                    for i in range(batchsize):
                        entry[i, :] = torch.from_numpy(q_t[i][k])
                else:
                    for i in range(batchsize):
                        entry[i, :] = q_t[i][k]

    # Put things on cuda.
    if use_cuda:
        _cpu2gpu(batch_cpu, batch_gpu, allow_incomplete_batch=allow_incomplete_batch)

class Pool:
    def __init__(self, num_pool):
        # Open a thread to assemble the batch.
        self.num_pool = num_pool
        self.pool = [ dict() for i in range(self.num_pool) ]

        self.empty_entries = Queue()
        for i in range(num_pool):
            self.pool[i]["_idx"] = i
            self.empty_entries.put(i)

    def reserve(self):
        idx = self.empty_entries.get()
        return self.pool[idx]

    def release(self, batch):
        self.empty_entries.put(batch["_idx"])


class BatchStats:
    def __init__(self, seq_limits=None):
        # Stats.
        self.stats_agent = Counter()
        self.seq_stats = SeqStats(seq_limits=seq_limits)

    def add_stats(self, batch):
        for agent_name in batch["_agent_name"]:
            self.stats_agent[agent_name] += 1

        self.seq_stats.feed(batch["_seq"])

    def print_stats(self):
        sum_counter = sum(self.stats_agent.values())
        num_agents = len(self.stats_agent)
        avg_counter = sum_counter / num_agents
        print("Agent Stats: %.3f[%d/%d]" % (avg_counter, sum_counter, num_agents))
        print(self.stats_agent.most_common(20))
        self.seq_stats.print_stats()

    def clear_stats(self):
        self.stats_agent.clear()
        self.seq_stats.clear_stats()


def ZMQDecoder(receive_data):
    sender_name, m = receive_data
    if sender_name is None:
        if m is None:
            # Done with the loop
            return "exit"
        else:
            # No package for now
            # send existing data if there is any.
            return "nopackage"

    try:
        m = loads(m.buffer)
    except:
        # If there is anything wrong with the decoding, return "nopackage"
        return "nopackage"

    sender_name = sender_name.bytes
    for data in m:
        data["_sender"] = sender_name

    return m

class MemoryReceiver:
    def __init__(self, name, ch, batch_assembler, batch_queue,
                 prompt=None, decoder=ZMQDecoder, allow_incomplete_batch=False,
                 seq_limits=None, replier=None):
        self.name = name
        self.ch = ch

        self.batch_assembler = batch_assembler
        self.loop_count = 0
        self.done_flag = None
        self.use_cuda = torch.cuda.is_available()

        self.batch_queue = batch_queue
        self.prompt = prompt
        self.allow_incomplete_batch = allow_incomplete_batch

        # BatchStats:
        self.batch_stats = BatchStats(seq_limits=seq_limits)
        self.decoder = decoder

        self.timer = Timer()
        # XXX Not a good design. Need to refactor
        self.replier = replier

        # Open a thread to assemble the batch.
        # TODO: For some reason, single threaded version is substantially
        #       faster than two-threaded version, which is supposed to hide
        #       the latency when receiving the data and build the batch.
        self.pool = Pool(2)
        threading.Thread(target=self._receive).start()

    def on_data(self, m):
        qs = self.batch_assembler.feed(m)
        if qs is not None:
            if self.prompt is not None:
                queue_size = self.batch_queue.qsize()
                print(self.prompt["on_draw_batch"] + str(queue_size), end="")
                sys.stdout.flush()

            self._make_and_send_batch(qs)

    def on_incomplete_batch(self):
        ''' Incomplete batch '''
        if self.batch_assembler.sample_count() == 0 or not self.allow_incomplete_batch: return
        qs = self.batch_assembler.get_batch(incomplete=True)
        if qs is not None:
            self._make_and_send_batch(qs)

    def _make_and_send_batch(self, qs):
        batch = self.pool.reserve()
        if self.prompt is not None:
            print(self.prompt["on_make_batch"], end="")
            sys.stdout.flush()
        _make_batch(batch, qs, use_cuda=self.use_cuda, allow_incomplete_batch=self.allow_incomplete_batch)
        self.batch_stats.add_stats(batch["cpu"][0])
        queue_put(self.batch_queue, (self, batch), done_flag=self.done_flag, fail_comment="BatchConnector.on_data.queue_put failed, retrying")

    def _preprocess(self, raw_data):
        ''' Return a list contains all the data to be fed to the assembler
            If return [], then no data are received.
            If return None, then we should exit.
        '''
        if self.decoder:
            with self.timer("decode"):
                m = self.decoder(raw_data)
            if isinstance(m, str):
                # No package for now, send existing data if there is any.
                if m == 'nopackage': return []
                else: return None
        else:
            if raw_data is None: return []
            else: m = raw_data

        # Send to multiple threads for batch.
        with self.timer("collect"):
            for data in m:
                data["_key"] = self._get_key(data)
                if not "_sender" in data:
                    data["_sender"] = data["_key"]

        return ret

    def _get_key(self, data):
        return "%s-%d-%d" % (data["_agent_name"], data["_game_counter"], data["_seq"])

    def _receive(self):
        check_interval = 200
        counter = 0
        while True:
            with self.timer("receive"):
                raw_data = self.ch.Receive()

            m =  self._preprocess(raw_data)
            if m is None: break

            if len(m) == 0:
                self.on_incomplete_batch()
                continue

            for data in m:
                self.loop_count += 1
                self.on_data(data)

            counter += 1
            if counter % check_interval == 0:
                # print("MemoryReceiver: %s, #data = %d" % (", ".join(self.timer.summary()), len(m)))
                if check_done_flag(self.done_flag): break

        print("Exit from MemoryReceiver._receive")

    def print_stats(self):
        queue_size = self.batch_queue.qsize()
        print("Queue size: %d" % queue_size)
        self.batch_stats.print_stats()
        self.batch_stats.clear_stats()

    def Step(self, batch, reply):
        # Send reply back and release
        if self.replier is not None:
            self.replier.Reply(batch, reply)

        self.pool.release(batch)
        if self.prompt is not None:
            print(self.prompt["on_release_batch"], end="")
            sys.stdout.flush()

