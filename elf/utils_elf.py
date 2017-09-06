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

class Batch:
    torch_types = {
        "int32_t" : torch.IntTensor,
        "int64_t" : torch.LongTensor,
        "float" : torch.FloatTensor,
        "unsigned char" : torch.ByteTensor,
        "char" : torch.ByteTensor
    }
    numpy_types = {
        "int32_t": 'i4',
        'int64_t': 'i8',
        'float': 'f4',
        'unsigned char': 'byte',
        'char': 'byte'
    }

    def __init__(self, **kwargs):
        self.batch = kwargs

    def _request(GC, group_id, key, T):
        info = GC.GetTensorSpec(group_id, key, T)
        # print("Key = \"%s\"" % str(key))
        if info.key == '':
            print(key + " is not found, try last_" + key)
            last_key = "last_" + key
            info = GC.GetTensorSpec(group_id, last_key, T)
            if info.key == '':
                raise ValueError("key[%s] or last_key[%s] is not specified!" % (key, last_key))
        return info

    def _alloc(info, use_gpu=True, use_numpy=True):
        if not use_numpy:
            v = Batch.torch_types[info.type](*info.sz)
            if use_gpu:
                v = v.pin_memory()
            v.fill_(1)
            info.p = v.data_ptr()
            info.byte_size = v.numel() * v.element_size()
        else:
            v = np.zeros(info.sz, dtype=Batch.numpy_types[info.type])
            v[:] = 1
            info.p = v.ctypes.data
            info.byte_size = v.size * v.dtype.itemsize

        return v, info

    def load(GC, input_reply, desc, group_id, use_gpu=True, use_numpy=False):
        batch = Batch()
        batch.infos = { }
        batch.GC = GC

        keys = desc["keys"]
        T = desc["T"]

        for key in keys:
            info = Batch._request(GC, group_id, key, T)
            v, info = Batch._alloc(info, use_gpu=use_gpu, use_numpy=use_numpy)
            batch.batch[info.key] = v
            batch.infos[info.key] = info
            GC.AddTensor(group_id, input_reply, info)

        return batch

    def __getitem__(self, key):
        if key in self.batch:
            return self.batch[key]
        else:
            key_with_last = "last_" + key
            if key_with_last in self.batch:
                return self.batch[key_with_last][1:]
            else:
                raise KeyError("Batch(): specified key: %s or %s not found!" % (key, key_with_last))

    def __contains__(self, key):
        return key in self.batch or "last_" + key in self.batch

    def setzero(self):
        for _, v in self.batch.items():
            v[:] = 0

    def copy_from(self, src):
        this_src = src if isinstance(src, dict) else src.batch

        for k, v in this_src.items():
            # Copy it down to cpu.
            if k in self.batch:
                bk = self.batch[k]
                if isinstance(v, list) and bk.numel() == len(v):
                    bk = bk.view(-1)
                    for i, vv in enumerate(v):
                        bk[i] = vv
                elif isinstance(v, (int, float)):
                    bk.fill_(v)
                else:
                    bk[:] = v

    def cpu2gpu(self, gpu=0):
        return Batch(**{ k : v.cuda(gpu) for k, v in self.batch.items() })

    def hist(self, s, key=None):
        '''s=1 means going back in time by one step, etc'''
        if key is None:
            new_batch = Batch(**{ k : v[s] for k, v in self.batch.items() })
            new_batch.GC = self.GC
            return new_batch
        else:
            return self[key][s]

    def transfer_cpu2gpu(self, batch_gpu, async=True):
        # For each time step
        for k, v in self.batch.items():
            batch_gpu[k].copy_(v, async=async)

    def transfer_cpu2cpu(self, batch_dst, async=True):
        # For each time step
        for k, v in self.batch.items():
            batch_dst[k].copy_(v)

    def pin_clone(self):
        return Batch(**{ k : v.clone().pin_memory() for k, v in self.batch.items() })

    def to_numpy(self):
        return { k : v.numpy() if not isinstance(v, np.ndarray) else v for k, v in self.batch.items() }


class GCWrapper:
    def __init__(self, GC, co, descriptions, use_numpy=False, gpu=None, params=dict()):
        '''Initialize GCWarpper

        Parameters:
            GC(C++ class): Game Context
            co(C type): context parameters.
            descriptions(list of tuple of dict): descriptions of input and reply entries.
            use_numpy(boolean): whether we use numpy array (or PyTorch tensors)
            gpu(int): gpu to use.
            params(dict): additional parameters
        '''

        self._init_collectors(GC, co, descriptions, use_gpu=gpu is not None, use_numpy=use_numpy)
        self.gpu = None
        self.inputs_gpu = None
        self.setup_gpu(gpu)
        self.params = params
        self._cb = { }

    def _init_collectors(self, GC, co, descriptions, use_gpu=True, use_numpy=False):
        num_games = co.num_games

        total_batchsize = 0
        for key, v in descriptions.items():
            total_batchsize += v["batchsize"]
        num_recv_thread = math.floor(num_games / total_batchsize)
        num_recv_thread = max(num_recv_thread, 1)
        print("#recv_thread = %d" % num_recv_thread)

        inputs = []
        replies = []
        idx2name = {}
        name2idx = defaultdict(list)

        gid2gpu = {}
        gpu2gid = []

        for key, v in descriptions.items():
            input = v["input"]
            reply = v["reply"]
            batchsize = v["batchsize"]
            T = input["T"]
            if reply is not None and reply["T"] > T:
                T = reply["T"]
            gstat = GC.CreateGroupStat()
            gstat.hist_len = T

            # If we specifiy filters, we need to put the info into gstat.
            filters = v.get("filters", {})
            gstat.player_name = filters.get("player_name", "")

            print("Deal with connector. key = %s, hist_len = %d, player_name = %s" % (key, gstat.hist_len, gstat.player_name))

            gpu2gid.append(list())
            for i in range(num_recv_thread):
                group_id = GC.AddCollectors(batchsize, len(gpu2gid) - 1, gstat)

                inputs.append(Batch.load(GC, "input", input, group_id, use_gpu=use_gpu, use_numpy=use_numpy))
                if reply is not None:
                    replies.append(Batch.load(GC, "reply", reply, group_id, use_gpu=use_gpu, use_numpy=use_numpy))
                else:
                    replies.append(None)

                idx2name[group_id] = key
                name2idx[key].append(group_id)
                gpu2gid[-1].append(group_id)
                gid2gpu[group_id] = len(gpu2gid) - 1

        # Zero out all replies.
        for reply in replies:
            if reply is not None:
                reply.setzero()

        self.GC = GC
        self.inputs = inputs
        self.replies = replies
        self.idx2name = idx2name
        self.name2idx = name2idx
        self.gid2gpu = gid2gpu
        self.gpu2gid = gpu2gid

    def setup_gpu(self, gpu):
        '''Setup the gpu used in the wrapper'''
        if gpu is not None and self.gpu != gpu:
            self.gpu = gpu
            self.inputs_gpu = [ self.inputs[gids[0]].cpu2gpu(gpu=gpu) for gids in self.gpu2gid ]

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
            sel.transfer_cpu2gpu(sel_gpu)
            picked = sel_gpu
        else:
            picked = sel

        # Get the reply array
        if len(self.replies) > infos.gid and self.replies[infos.gid] is not None:
            sel_reply = self.replies[infos.gid]
        else:
            sel_reply = None

        # Call
        if infos.gid in self._cb:
            reply = self._cb[infos.gid](picked)
            # If reply is meaningful, send them back.
            if isinstance(reply, dict) and sel_reply is not None:
                # Current we only support reply to the most recent history.
                sel_reply.copy_from(reply)

    def Run(self):
        '''Wait group of an arbitrary collector key. Samples in a returned batch are always from the same group, but the group key of the batch may be arbitrary.'''
        # print("before wait")
        self.infos = self.GC.Wait(0)
        # print("before calling")
        res = self._call(self.infos)
        # print("before_step")
        self.GC.Steps(self.infos)
        return res

    def Start(self):
        '''Start all game environments'''
        self.GC.Start()

    def Stop(self):
        '''Stop all game environments. :func:`Start()` cannot be called again after :func:`Stop()` has been called.'''
        self.GC.Stop()

    def reg_sig_int(self):
        import signal
        def signal_handler(s, frame):
            print('Detected Ctrl-C!')
            self.Stop()
            sys.exit(0)
        signal.signal(signal.SIGINT, signal_handler)

    def PrintSummary(self):
        '''Print summary'''
        self.GC.PrintSummary()
