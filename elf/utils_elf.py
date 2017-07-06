# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import torch
import sys
import math
import numpy as np

def cpu2gpu(batch, gpu=0):
    ''' Preallocation '''
    batch_gpu = [dict() for i in range(len(batch))]
    # Use the first time step
    for i, bt in enumerate(batch):
        for k, v in bt.items():
            batch_gpu[i][k] = v.cuda(gpu)

    return batch_gpu

def transfer_cpu2gpu(batch, batch_gpu, async=True):
    # For each time step
    for bt, bt_gpu in zip(batch, batch_gpu):
        for k, v in bt.items():
            bt_gpu[k].copy_(v, async=async)

def transfer_cpu2cpu(batch, batch_dst, async=True):
    # For each time step
    for bt, bt_dst in zip(batch, batch_dst):
        for k, v in bt.items():
            bt_dst[k].copy_(v)


def pin_clone(batch):
    batch_cloned = []
    for i, bt in enumerate(batch):
        batch_cloned.append(dict())
        for k, v in bt.items():
            batch_cloned[i][k] = v.clone().pin_memory()
    return batch_cloned


def print_ptrs(batches):
    for thread_id, batch in enumerate(batches):
        for t, bt in enumerate(batch):
            for k, v in bt.items():
                print("[thread=%d][t=%d] %s: %x" % (thread_id, t, k, v.data_ptr()))


def _setup_tensor(GC, key, desc, group_id, num_thread, use_numpy=False):
    torch_types = {
        "int" : torch.IntTensor,
        "int64_t" : torch.LongTensor,
        "float" : torch.FloatTensor,
        "unsigned char" : torch.ByteTensor
    }
    numpy_types = {
        "int": 'i4',
        'int64_t': 'i8',
        'float': 'f4',
        'unsigned char': 'byte'
    }

    batches = []
    T = int(desc["_T"])

    for i in range(num_thread):
        n = GC.CreateTensor(group_id, i, key, desc)
        batch = [dict() for t in range(T)]

        # print("thread = %d" % i)

        # Then we get
        for j in range(n):
            info = GC.GetTensorInfo(group_id, i, key, j)
            # Then we use the info to create the tensor.
            if not use_numpy:
                v = torch_types[info.type](*info.sz).pin_memory()
                p = v.data_ptr()
                stride = v.stride()[0]
            else:
                v = np.zeros(info.sz, dtype=numpy_types[info.type])
                p = v.ctypes.data
                stride = v.ctypes.strides[0] // v.dtype.itemsize
            # print(info.key + " " + str(info.sz) + " addr: " + str(p) + " stride: " + str(stride))

            # Then we set the tensor address and stride.
            GC.SetTensorAddr(group_id, i, key, j, p, stride)
            batch[info.hist_loc_for_py][info.key] = v

        batches.append(batch)

    return batches

def to_numpy_t(bt):
    return { k : v.numpy() if not isinstance(v, np.ndarray) else v for k, v in bt.items() }

def to_numpy(batch):
    return [ to_numpy_t(bt) for bt in batch ]


def init_collectors(GC, co, descriptions, use_numpy=False):
    # batch_input, batch_reply, data_addrs = get_prealloc_data_n(desc_input, desc_reply)
    #   desc_input = {"_batchsize" : str(args.batchsize), "_T" : str(history_len), "s" : "4" , "pi" : "6", "a": "1", "r": "1"})
    #
    # The output would be:
    #     batch = [dict(s=, pi=, r=), dict(s=, pi=, r=), ... dict(s=, pi=, r=)]
    # where len(batch) = history_len
    #     batch[0]["s"] = FloatTensor(batchsize, value(s) * 3, 105, 80)
    #         value(s) is the length of the history
    #     batch[0]["pi"] = FloatTensor(batchsize, value(a))
    #         value(a) is length of the action.
    #     batch[0]["r"] = FloatTensor(batchsize) -> clip(raw_reward, value(r))
    #     batch[0]["a"] = IntTensor(batchsize, value(a))
    #         sampled action.

    num_games = co.num_games

    total_batchsize = 0
    for input, _ in descriptions:
        if "_batchsize" not in input or input["_batchsize"] is None:
            print("Batchsize cannot be None!")
            sys.exit(1)
        total_batchsize += int(input["_batchsize"])
    num_recv_thread = math.floor(num_games / total_batchsize)
    num_recv_thread = max(num_recv_thread, 1)

    inputs = []
    replies = []
    for input, reply in descriptions:
        batchsize = int(input["_batchsize"])
        T = int(input["_T"])
        group_id = GC.AddCollectors(batchsize, T, num_recv_thread)
        inputs.append(_setup_tensor(GC, "input", input, group_id, num_recv_thread, use_numpy=use_numpy))
        if reply is not None:
            replies.append(_setup_tensor(GC, "reply", reply, group_id, num_recv_thread, use_numpy=use_numpy))
        else:
            replies.append(None)

    # Print pointers.
    '''print("Ptrs for batches_train_input")
    print_ptrs(batches_train_input)

    print("Ptrs for batches_actor_input")
    print_ptrs(batches_actor_input)
    print("Ptrs for batches_actor_reply")
    print_ptrs(batches_actor_reply)
    '''

    return inputs, replies

class GCWrapper:
    def __init__(self, GC, inputs, replies, name2idx, params, gpu=0):
        self.GC = GC
        self.inputs = inputs
        self.replies = replies
        self.name2idx = name2idx
        self.params = params
        self.inputs_gpu = None
        self._cb = { }

    def setup_gpu(self, gpu):
        self.gpu = gpu
        self.inputs_gpu = [ cpu2gpu(batch[0], gpu=gpu) for batch in self.inputs ]

    def reg_callback(self, key, cb):
        if key not in self.name2idx:
            return False
        self._cb[self.name2idx[key]] = cb
        return True

    def _call(self, infos):
        sel = self.inputs[infos.gid][infos.id_in_group]
        if self.inputs_gpu is not None:
            sel_gpu = self.inputs_gpu[infos.gid]
            transfer_cpu2gpu(sel, sel_gpu)
        else:
            sel_gpu = None
        if len(self.replies) > infos.gid and self.replies[infos.gid] is not None:
            reply = self.replies[infos.gid][infos.id_in_group]
        else:
            reply = None

        # Call
        if infos.gid in self._cb:
            return self._cb[infos.gid](sel, sel_gpu, reply)

    def RunGroup(self, key):
        self.infos = self.GC.WaitGroup(self.name2idx[key], 0)
        res = self._call(self.infos)
        self.GC.Steps(self.infos)
        return res

    def Run(self):
        self.infos = self.GC.Wait(0)
        res = self._call(self.infos)
        self.GC.Steps(self.infos)
        return res

    def Start(self):
        self.GC.Start()

    def Stop(self):
        self.GC.Stop()

    def PrintSummary(self):
        self.GC.PrintSummary()
