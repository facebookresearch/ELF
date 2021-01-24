# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import torch
import sys
import math
import numpy as np
from collections import defaultdict

class Batch:
    ''' A wrapper class for batch data'''
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

    def __init__(self, _batchsize=None, **kwargs):
        ''' Initialize `Batch` class. Pass in a dict and wrap it into ``self.batch``'''
        if isinstance(_batchsize, int):
            self.batch = { k : v[:, :_batchsize] for k, v in kwargs.items() }
        else:
            self.batch = kwargs

    def first_k(self, batchsize):
        return Batch(_batchsize=batchsize, **self.batch)

    @staticmethod
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

    @staticmethod
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

    @staticmethod
    def load(GC, input_reply, desc, group_id, use_gpu=True, use_numpy=False):
        '''load Batch from the specifications

        Args:
            GC(C++ class): Game Context
            input_reply(str): ``"input"`` or ``"reply"`` to indicate input batch data or reply batch data
            desc(dict): description of the batch we want. Detailed explanation can be see in :doc:`wrapper-python`. The Python interface of wrapper.
            group_id(int): group id. Batch data with the same group id will be batched together
            use_gpu(bool): indicates if we use gpu
            use_numpy(bool): indicates if we use numpy

        Returns:
            loaded batch object
        '''
        batch = Batch()
        batch.dims = { }
        batch.GC = GC

        keys = desc["keys"]
        T = desc["T"]

        for key in keys:
            info = Batch._request(GC, group_id, key, T)
            v, info = Batch._alloc(info, use_gpu=use_gpu, use_numpy=use_numpy)
            batch.batch[info.key] = v
            batch.dims[info.key] = info
            # print(key + " : " + str(v.size()))
            GC.AddTensor(group_id, input_reply, info)

        return batch

    def __getitem__(self, key):
        ''' Get a key from batch. Can be either ``key`` or ``last_key``

        Args:
            key(str): key name. e.g. if ``r`` is passed in, will search for ``r`` or ``last_r``
        '''
        if key in self.batch:
            return self.batch[key]
        else:
            key_with_last = "last_" + key
            if key_with_last in self.batch:
                return self.batch[key_with_last][1:]
            else:
                raise KeyError("Batch(): specified key: %s or %s not found!" % (key, key_with_last))

    def add(self, key, value):
        '''
        Add key=value in Batch. This is used when you want to send additional state to the
        learning algorithm, e.g., hidden state collected from the previous iterations.
        '''
        self.batch[key] = value
        return self

    def __contains__(self, key):
        return key in self.batch or "last_" + key in self.batch

    def setzero(self):
        ''' Set all tensors in the batch to 0 '''
        for _, v in self.batch.items():
            v[:] = 0

    def copy_from(self, src, batch_key=""):
        ''' copy all keys and values from another dict or `Batch` object

        Args:
            src(dict or `Batch`): batch data to be copied
        '''
        this_src = src if isinstance(src, dict) else src.batch
        key_assigned = { k : False for k in self.batch.keys() }
        # import pdb
        # pdb.set_trace()
        for k, v in this_src.items():
            # Copy it down to cpu.
            if k in self.batch:
                bk = self.batch[k]
                key_assigned[k] = True
                if v is None:
                    continue
                if isinstance(v, list) and bk.numel() == len(v):
                    bk = bk.view(-1)
                    for i, vv in enumerate(v):
                        bk[i] = vv
                elif isinstance(v, (int, float)):
                    bk.fill_(v)
                else:
                    bk[:] = v

            else:
                raise ValueError("Batch[%s]: \"%s\" in reply is missing in batch specification" % (batch_key, k))

        # Check whether there is any key missing.
        for k, assigned in key_assigned.items():
            if not assigned:
                raise ValueError("Batch[%s].copy_from: Reply[%s] is not assigned" % (batch_key, k))

    def cpu2gpu(self, gpu=0):
        ''' call ``cuda()`` on all batch data

        Args:
            gpu(int): gpu id
        '''
        batch = Batch(**{ k : v.cuda(gpu) for k, v in self.batch.items() })
        batch.GC = self.GC
        return batch

    def hist(self, s, key=None):
        '''
        return batch history.

        Args:
            s(int): s=1 means going back in time by one step, etc
            key(str): if None, return all key's history, otherwise just return that key's history
        '''
        if key is None:
            new_batch = Batch(**{ k : v[s] for k, v in self.batch.items() })
            if hasattr(self, "GC"):
                new_batch.GC = self.GC
            return new_batch
        else:
            return self[key][s]

    def transfer_cpu2gpu(self, batch_gpu, async=True):
        ''' transfer batch data to gpu '''
        # For each time step
        for k, v in self.batch.items():
            batch_gpu[k].copy_(v, async=async)

    def transfer_cpu2cpu(self, batch_dst, async=True):
        ''' transfer batch data to cpu '''

        # For each time step
        for k, v in self.batch.items():
            batch_dst[k].copy_(v)

    def pin_clone(self):
        ''' clone and pin memory for faster transportations to gpu '''

        return Batch(**{ k : v.clone().pin_memory() for k, v in self.batch.items() })

    def to_numpy(self):
        ''' convert batch data to numpy format '''
        return { k : v.numpy() if not isinstance(v, np.ndarray) else v for k, v in self.batch.items() }


class GCWrapper:
    def __init__(self, GC, co, descriptions, use_numpy=False, gpu=None, params=dict()):
        '''Initialize GCWarpper

        Parameters:
            GC(C++ class): Game Context
            co(C type): context parameters.
            descriptions(list of tuple of dict): descriptions of input and reply entries. Detailed explanation can be see in :doc:`wrapper-python`. The Python interface of wrapper.
            use_numpy(boolean): whether we use numpy array (or PyTorch tensors)
            gpu(int): gpu to use.
            params(dict): additional parameters
        '''
 
        #self.isPrint = False
        self._init_collectors(GC, co, descriptions, use_gpu=gpu is not None, use_numpy=use_numpy)
        self.gpu = gpu
        self.inputs_gpu = [ self.inputs[gids[0]].cpu2gpu(gpu=gpu) for gids in self.gpu2gid ] if gpu is not None else None
        self.params = params
        self._cb = { }
        

    def _init_collectors(self, GC, co, descriptions, use_gpu=True, use_numpy=False):
        num_games = co.num_games

        total_batchsize = 0
        for key, v in descriptions.items():
            total_batchsize += v["batchsize"]
        
        if co.num_collectors > 0:
            num_recv_thread = co.num_collectors
        else:
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

            gstat.name = v.get("name", "")
            timeout_usec = v.get("timeout_usec", 0)

            gpu2gid.append(list())
            for i in range(num_recv_thread):
                group_id = GC.AddCollectors(batchsize, len(gpu2gid) - 1, timeout_usec, gstat)

                input_batch = Batch.load(GC, "input", input, group_id, use_gpu=use_gpu, use_numpy=use_numpy) # 加载输入Batch
                input_batch.batchsize = batchsize
                inputs.append(input_batch)
                if reply is not None:
                    reply_batch = Batch.load(GC, "reply", reply, group_id, use_gpu=use_gpu, use_numpy=use_numpy) # 加载回复Batch
                    reply_batch.batchsize= batchsize
                    replies.append(reply_batch)
                else:
                    replies.append(None)

                idx2name[group_id] = key
                name2idx[key].append(group_id)
                gpu2gid[-1].append(group_id)
                gid2gpu[group_id] = len(gpu2gid) - 1

        print(GC.GetCollectorInfos())

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
        # if not self.isPrint:
            # print("idx2name",self.idx2name)
            # print("name2idx",self.name2idx)
            # print("gid2gpu",self.gid2gpu)
            # print("gpu2gid",self.gpu2gid)
            # print("num_collectors: ",co.num_collectors)
            # self.isPrint = True


    def reg_has_callback(self, key):
        return key in self.name2idx

    def reg_callback_if_exists(self, key, cb):
        if self.reg_has_callback(key):
            self.reg_callback(key, cb)
            return True
        else:
            return False

    def reg_callback(self, key, cb):
        '''Set callback function for key
        注册回调函数，有符合要求和数量的Batch到来时，调用对应的函数

        Parameters:
            key(str): the key used to register the callback function.
              If the key is not present in the descriptions, return ``False``.
            cb(function): the callback function to be called.
              The callback function has the signature ``cb(input_batch, input_batch_gpu, reply_batch)``.
        '''
        if key not in self.name2idx:
            raise ValueError("Callback[%s] is not in the specification" % key)
        if cb is None:
            print("Warning: Callback[%s] is registered to None" % key)

        for gid in self.name2idx[key]:
            self._cb[gid] = cb
        return True

    def _call(self, infos):
        if infos.gid not in self._cb:
            raise ValueError("info.gid[%d] is not in callback functions" % infos.gid)

        if self._cb[infos.gid] is None:
            return

        batchsize = len(infos.s)

        sel = self.inputs[infos.gid].first_k(batchsize)
        if self.inputs_gpu is not None:
            sel_gpu = self.inputs_gpu[self.gid2gpu[infos.gid]].first_k(batchsize)
            sel.transfer_cpu2gpu(sel_gpu)
            picked = sel_gpu
        else:
            picked = sel

        # Save the infos structure, if people want to have access to state
        # directly, they can use infos.s[i], which is a state pointer.
        picked.infos = infos
        picked.batchsize = batchsize
        picked.max_batchsize = self.inputs[infos.gid].batchsize

        # Get the reply array
        if len(self.replies) > infos.gid and self.replies[infos.gid] is not None:
            sel_reply = self.replies[infos.gid].first_k(batchsize)
        else:
            sel_reply = None

        reply = self._cb[infos.gid](picked)
        # If reply is meaningful, send them back.
        if isinstance(reply, dict) and sel_reply is not None:
            # Current we only support reply to the most recent history.
            batch_key = "%s-%d" % (self.idx2name[infos.gid], infos.gid)
            sel_reply.copy_from(reply, batch_key=batch_key)

    def _check_callbacks(self):
        # Check whether all callbacks are assigned properly.
        for key, gids in self.name2idx.items():
            for gid in gids:
                if gid not in self._cb:
                    raise ValueError("GCWrapper.Start(): No callback function for key = %s and gid = %d" % (key, gid))

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
        self._check_callbacks()
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
