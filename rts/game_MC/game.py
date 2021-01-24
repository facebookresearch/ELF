# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import minirts
from datetime import datetime

import sys, os

sys.path.append('../../')

from rts.engine import CommonLoader
from rlpytorch import ArgsProvider

class Loader(CommonLoader):
    def __init__(self):
        super(Loader, self).__init__(minirts)

    def _define_args(self):
        return [
            ("use_unit_action", dict(action="store_true")),     # false
            ("disable_time_decay", dict(action="store_true")),  # false
            ("use_prev_units", dict(action="store_true")),      # false 
            ("attach_complete_info", dict(action="store_true")),# false
            ("feature_type", "ORIGINAL")                        # "ORIGINAL"
        ]

    def _on_gc(self, GC):
        opt = minirts.MCExtractorOptions()
        opt.use_time_decay = not self.args.disable_time_decay
        opt.save_prev_seen_units = self.args.use_prev_units
        opt.attach_complete_info = self.args.attach_complete_info
        GC.ApplyExtractorParams(opt)  # 设置 MCExtractor

        usage = minirts.MCExtractorUsageOptions()
        usage.Set(self.args.feature_type)
        GC.ApplyExtractorUsage(usage) # 设置 ExtractorUsage

    def _unit_action_keys(self):
        if self.args.use_unit_action:
            return ["uloc", "tloc", "bt", "ct", "uloc_prob", "tloc_prob", "bt_prob", "ct_prob"]
        else:
            return ["pi","a"]

    def _get_actor_spec(self): #d=定义用于actor的batch字典
        reply_keys = ["V","action_type"]  # unit_cmd 的回复

        return dict(
            batchsize=self.args.batchsize,
            input=dict(T=1, keys=set(["s", "last_r", "terminal"])),  # 期待收到 s last_r terminal
            reply=dict(T=1, keys=set(reply_keys + self._unit_action_keys())),
        )

    def _get_train_spec(self):
        keys = ["s", "last_r", "V", "terminal", "pi", "a"]
        return dict(
            batchsize=self.args.batchsize,
            input=dict(T=self.args.T, keys=set(keys + self._unit_action_keys())),
            reply=None  # train 不需要回复
        )

    def _get_reduced_predict(self):
        return dict(
            batchsize=self.args.batchsize,
            input=dict(T=1, keys=set(["reduced_s"])),
            reply=dict(T=1, keys=set(["pi", "V"])),
            name="reduced_predict",
            timeout_usec=100
        )

    def _get_reduced_forward(self):
        return dict(
            batchsize=self.args.batchsize,
            input=dict(T=1, keys=set(["reduced_s", "a"])),
            reply=dict(T=1, keys=set(["reduced_next_s"])),
            name="reduced_forward",
            timeout_usec=100
        )

    def _get_reduced_project(self):
        return dict(
            batchsize=min(self.args.batchsize, max(self.args.num_games // 2, 1)),
            input=dict(T=1, keys=set(["s", "last_r"])),
            reply=dict(T=1, keys=set(["reduced_s"])),
            name="reduced_project"
        )

nIter = 5000
elapsed_wait_only = 0

import pickle
import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    loader = Loader()
    args = ArgsProvider.Load(parser, [loader])

    cnt_predict = 0
    cnt_forward = 0
    cnt_project = 0

    def actor(batch):
        '''
        import pdb
        pdb.set_trace()
        pickle.dump(utils_elf.to_numpy(sel), open("tmp%d.bin" % k, "wb"), protocol=2)
        '''
        return dict(a=[0]*batch["s"].size(1))

    def reduced_predict(batch):
        global cnt_predict
        cnt_predict += 1
        # print("in reduced_predict, cnt_predict = %d" % cnt_predict)

    def reduced_forward(batch):
        global cnt_forward
        cnt_forward += 1
        # print("in reduced_forward, cnt_forward = %d" % cnt_forward)

    def reduced_project(batch):
        global cnt_project
        cnt_project += 1
        # print("in reduced_project, cnt_project = %d" % cnt_project)

    # GC = loader.initialize()
    GC = loader.initialize_reduced_service()
    GC.reg_callback("actor", actor)
    GC.reg_callback("reduced_predict", reduced_predict)
    GC.reg_callback("reduced_forward", reduced_forward)
    GC.reg_callback("reduced_project", reduced_project)

    before = datetime.now()
    GC.Start()

    import tqdm
    for k in tqdm.trange(nIter):
        b = datetime.now()
        GC.Run()
        elapsed_wait_only += (datetime.now() - b).total_seconds() * 1000
        #img = np.array(infos[0].data.image, copy=False)

    print("#predict: %d, #forward: %d, #project: %d" % (cnt_predict, cnt_forward, cnt_project))

    elapsed = (datetime.now() - before).total_seconds() * 1000
    print("elapsed = %.4lf ms, elapsed_wait_only = %.4lf" % (elapsed, elapsed_wait_only))
    GC.PrintSummary()
    GC.Stop()

    # Compute the statistics.
    per_loop = elapsed / nIter
    per_wait = elapsed_wait_only / nIter
    per_frame_loop_n_cpu = per_loop / args.batchsize
    per_frame_wait_n_cpu = per_wait / args.batchsize

    fps_loop = 1000 / per_frame_loop_n_cpu * args.frame_skip
    fps_wait = 1000 / per_frame_wait_n_cpu * args.frame_skip

    print("Time[Loop]: %.6lf ms / loop, %.6lf ms / frame_loop_n_cpu, %.2f FPS" % (per_loop, per_frame_loop_n_cpu, fps_loop))
    print("Time[Wait]: %.6lf ms / wait, %.6lf ms / frame_wait_n_cpu, %.2f FPS" % (per_wait, per_frame_wait_n_cpu, fps_wait))
