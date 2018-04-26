# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

from .assembler import ExpOp, BatchAssembler
from .replier import Replier
from .memory_receiver import MemoryReceiver
from .zmq_adapter import InitConnector
from queue import Queue

class Collector:
    def __init__(self, name, collector_name, batchsize, T, batch_queue):
        self.receiver = InitConnector(collector_name)
        self.exp_op = ExpOp(use_future=False)
        self.assembler = BatchAssembler(batchsize, self.exp_op, T=T)

        self.replier = Replier(self.receiver, reply_batchsize=1)

        # XXX Not a good design to put replier into memory_receiver.
        # For now let it go.
        self.memory_receiver = \
            MemoryReceiver(name, self.receiver, self.assembler, batch_queue, allow_incomplete_batch=False, replier=self.replier)

class GCWrapper:
    def __init__(self, simulator_class, desc, num_games, batchsize, T):
        self.desc = desc
        self.batch_q = Queue()
        self.collectors = [ Collector(key, v["connector"], batchsize, T, self.batch_q) for key, v in self.desc.items() ]

        self.simulators = [ simulator_class(str(i), desc) for i in range(num_games) ]
        for simulator in self.simulators:
            simulator.start()

        self._cb = { }

    def reg_callback(self, key, callback):
        self._cb[key] = callback

    def Run(self):
        rv, batch = self.batch_q.get()

        reply = None
        if rv.name in self._cb:
            reply = self._cb[rv.name](batch["cpu"], batch["gpu"])

        rv.Step(batch, reply)

