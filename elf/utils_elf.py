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
from collections import defaultdict

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


def _setup_tensor(GC, key, desc, group_id, use_numpy=False):
    torch_types = {
        "int32_t" : torch.IntTensor,
        "int64_t" : torch.LongTensor,
        "float" : torch.FloatTensor,
        "unsigned char" : torch.ByteTensor
    }
    numpy_types = {
        "int32_t": 'i4',
        'int64_t': 'i8',
        'float': 'f4',
        'unsigned char': 'byte'
    }

    T = int(desc["_T"])

    n = GC.CreateTensor(group_id, key, desc)
    batch = [dict() for t in range(T)]

    # print("thread = %d" % i)

    # Then we get
    for j in range(n):
        info = GC.GetTensorInfo(group_id, key, j)
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
        GC.SetTensorAddr(group_id, key, j, p, stride)
        batch[info.hist_loc_for_py][info.key] = v

    return batch

def to_numpy_t(bt):
    return { k : v.numpy() if not isinstance(v, np.ndarray) else v for k, v in bt.items() }

def to_numpy(batch):
    return [ to_numpy_t(bt) for bt in batch ]


class GCWrapper:
    def __init__(self, GC, co, descriptions, use_numpy=False, gpu=0, params=dict()):
        '''Initialize GCWarpper

        Parameters:
            GC(C++ class): Game Context
            co(C type): context parameters.
            descriptions(list of tuple of dict): descriptions of input and reply entries.
            use_numpy(boolean): whether we use numpy array (or PyTorch tensors)
            gpu(int): gpu to use.
            params(dict): additional parameters
        '''

        self._init_collectors(GC, co, descriptions, use_numpy=use_numpy)
        self.gpu = gpu
        self.inputs_gpu = None
        self.params = params
        self._cb = { }

    def _init_collectors(self, GC, co, descriptions, use_numpy=False):
        num_games = co.num_games

        total_batchsize = 0
        for key, (input, reply) in descriptions.items():
            if "_batchsize" not in input or input["_batchsize"] is None:
                print("Batchsize cannot be None!")
                sys.exit(1)
            total_batchsize += int(input["_batchsize"])
        num_recv_thread = math.floor(num_games / total_batchsize)
        num_recv_thread = max(num_recv_thread, 1)

        inputs = []
        replies = []
        idx2name = {}
        name2idx = defaultdict(list)

        gid2gpu = {}
        gpu2gid = []

        for key, (input, reply) in descriptions.items():
            batchsize = int(input["_batchsize"])
            T = int(input["_T"])
            gpu2gid.append(list())
            for i in range(num_recv_thread):
                group_id = GC.AddCollectors(batchsize, T, len(gpu2gid) - 1)
                inputs.append(_setup_tensor(GC, "input", input, group_id, use_numpy=use_numpy))
                if reply is not None:
                    replies.append(_setup_tensor(GC, "reply", reply, group_id, use_numpy=use_numpy))
                else:
                    replies.append(None)
                idx2name[group_id] = key
                name2idx[key].append(group_id)
                gpu2gid[-1].append(group_id)
                gid2gpu[group_id] = len(gpu2gid) - 1

        # Zero out all replies.
        for reply in replies:
            if reply is not None:
                for r in reply:
                    for _, v in r.items():
                        v[:] = 0

        self.GC = GC
        self.inputs = inputs
        self.replies = replies
        self.idx2name = idx2name
        self.name2idx = name2idx
        self.gid2gpu = gid2gpu
        self.gpu2gid = gpu2gid

    def setup_gpu(self, gpu):
        '''Setup the gpu used in the wrapper'''
        self.gpu = gpu
        self.inputs_gpu = [ cpu2gpu(self.inputs[gids[0]], gpu=gpu) for gids in self.gpu2gid ]

    def reg_callback(self, key, cb):
        '''Set callback function for key

        Parameters:
            key(str): the key used to register the callback function.
              If the key is not present in the descriptions, return ``False``.
            cb(function): the callback function to be called.
              The callback function has the signature ``cb(input_batch, input_batch_gpu, reply_batch)``.
        '''
        if key not in self.name2idx:
            return False
        for gid in self.name2idx[key]:
            self._cb[gid] = cb
        return True

    def _call(self, infos):
        sel = self.inputs[infos.gid]
        if self.inputs_gpu is not None:
            sel_gpu = self.inputs_gpu[self.gid2gpu[infos.gid]]
            transfer_cpu2gpu(sel, sel_gpu)
        else:
            sel_gpu = None
        if len(self.replies) > infos.gid and self.replies[infos.gid] is not None:
            reply = self.replies[infos.gid]
        else:
            reply = None

        # Call
        if infos.gid in self._cb:
            return self._cb[infos.gid](sel, sel_gpu, reply)

    def Run(self):
        '''Wait group of an arbitrary collector key. Samples in a returned batch are always from the same group, but the group key of the batch may be arbitrary.'''
        self.infos = self.GC.Wait(0)
        res = self._call(self.infos)
        self.GC.Steps(self.infos)
        return res

    def Start(self):
        '''Start all game environments'''
        self.GC.Start()

    def Stop(self):
        '''Stop all game environments. :func:`Start()` cannot be called again after :func:`Stop()` has been called.'''
        self.GC.Stop()

    def PrintSummary(self):
        '''Print summary'''
        self.GC.PrintSummary()
