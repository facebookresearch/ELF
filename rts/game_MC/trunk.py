# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.autograd import Variable


from rlpytorch import Model
from collections import Counter

class MiniRTSNet(Model):
    def __init__(self, args, output1d=True):
        # this is the place where you instantiate all your modules
        # you can later access them using the same names you've given them in here
        super(MiniRTSNet, self).__init__(args)
        self._init(args)
        self.output1d = output1d
        #self.isPrint = False

    def _init(self, args):
        self.m = args.params.get("num_planes_per_time_stamp", 13)
        self.input_channel = args.params.get("num_planes", self.m)
        self.mapx = args.params.get("map_x", 20)
        self.mapy = args.params.get("map_y", 20)
        self.pool = nn.MaxPool2d(2, 2)

        # self.arch = "ccpccp"
        # self.arch = "ccccpccccp"
        if self.args.arch[0] == "\"" and self.args.arch[-1] == "\"":
            self.arch = self.args.arch[1:-1]
        else:
            self.arch = self.args.arch
        self.arch, channels = self.arch.split(";")

        self.num_channels = []
        for i, v in enumerate(channels.split(",")):
            if v == "-":
                self.num_channels.append(self.m if i > 0 else self.input_channel)
            else:
                self.num_channels.append(int(v))

        self.convs = [ nn.Conv2d(self.num_channels[i], self.num_channels[i+1], 3, padding = 1) for i in range(len(self.num_channels)-1) ]
        for i, conv in enumerate(self.convs):
            setattr(self, "conv%d" % (i + 1), conv)

        self.relu = nn.ReLU() if self._no_leaky_relu() else nn.LeakyReLU(0.1)

        if not self._no_bn():
            self.convs_bn = [ nn.BatchNorm2d(conv.out_channels) for conv in self.convs ]
            for i, conv_bn in enumerate(self.convs_bn):
                setattr(self, "conv%d_bn" % (i + 1), conv_bn)

    def _no_bn(self):
        return getattr(self.args, "disable_bn", False)

    def _no_leaky_relu(self):
        return getattr(self.args, "disable_leaky_relu", False)

    def get_define_args():
        return [
            ("arch", "ccpccp;-,64,64,64,-")
        ]

    def forward(self, input):
        # BN and LeakyReLU are from Wendy's code.
        x = input.view(input.size(0), self.input_channel, self.mapy, self.mapx)
        counts = Counter()
        for i in range(len(self.arch)):
            if self.arch[i] == "c":
                c = counts["c"]
                x = self.convs[c](x)
                if not self._no_bn(): x = self.convs_bn[c](x)
                x = self.relu(x)
                counts["c"] += 1
            elif self.arch[i] == "p":
                x = self.pool(x)

        if self.output1d:
            x = x.view(x.size(0), -1)
        
        # if not self.isPrint:
        #     print("x",x)
        #     print("x.size ",x.size())
        #     self.isPrint = True
        
        return x     # 64 x 550


class ProcessSet(Model):
    def __init__(self, args,MaxNum,d_model,d_ff):
        # this is the place where you instantiate all your modules
        # you can later access them using the same names you've given them in here
        super(ProcessSet, self).__init__(args)
        self._init(MaxNum,d_model,d_ff)

    def _init(self, MaxNum,d_model,d_ff):
        self.d_model = d_model
        self.d_ff = d_ff
        self.MaxNum = MaxNum
        self.w_1 = nn.Linear(d_model,d_ff)
        self.w_2 = nn.Linear(d_ff,d_model)
        self.pool = nn.MaxPool2d((MaxNum,1))
        


    def forward(self, input):
        #x = input.view(input.size(0), self.MaxNum, self.diff)
        # import pdb
        # pdb.set_trace()
        data_embedding =  F.relu(self.w_1(input))  # 激励函数(隐藏层的线性值)
        data_embedding =  self.w_2(data_embedding)  # 用于Attention
        data_single = self.pool(data_embedding)    # 描述整个集合
        return data_single,data_embedding

class DataProcess(Model):
    def __init__(self,args,baseProcesser,radarProcesser,towerProcesser,enemyProcessor):
        super(DataProcess, self).__init__(args)
        self.baseProcesser = baseProcesser
        self.radarProcesser = radarProcesser
        self.towerProcesser= towerProcesser
        self.enemyProcesser = enemyProcessor
        self.FC = nn.Linear(24,64)
        self.lstm = nn.LSTM(
            input_size=64,  # 输入数据
            hidden_size=256,  # rnn hidden unit
            num_layers=1,  # 有几层 RNN layers
            batch_first=True,  # input & output 会是以 batch size 为第一维度的特征集 e.g. (batch, time_step, input_size)
        )


    def forward(self,x):
        # import pdb
        # pdb.set_trace()
        radar,_ = self.radarProcesser(self._var(x["s_radar"])) # 2 x 3 
        tower,tower_embedding = self.towerProcesser(self._var(x["s_tower"])) # 1 x 8  16 x 8
        enemy,enemy_embedding = self.enemyProcesser(self._var(x["s_enemy"])) # 1 x 7  20 x 7
        base = self._var(x["s_base"]).view(-1,1,3)
        base,_ = self.baseProcesser(base)  # 1 x 3
        glo = self._var(x["s_global"]).view(-1,1,3) # 1 x 3
        global_stat = torch.cat([glo,base,radar,tower,enemy],2)  # 1 x 24
        global_stat = F.relu(self.FC(global_stat))
        global_stat = global_stat.view(-1,1,64)

        r_out, (h_n,h_c) = self.lstm(global_stat,None)  # None表示 h_0 和 c_0 置 0
        real_out_put = r_out[:, -1, :]


        return base,radar,tower,tower_embedding,enemy,enemy_embedding,real_out_put
# 满足条件赋值为
def where(cond, x_1, x_2):
    cond = cond.float()
    return (cond * x_1) + ((1-cond) * x_2)

class TargetAttention(Model):
    def __init__(self,args,in_dim,query_dim):
        super(TargetAttention, self).__init__(args)
        self.in_dim = in_dim
        self.query_dim = query_dim
        self.FC = nn.Linear(in_dim,query_dim)

    def forward(self,x,enemy_embedding,enemy_raw,use_mask = False):
        
        #assert enemy_embedding.shape[1] == self.query_dim
        
        query = self.FC(x)
        query = query.view(-1,1,self.query_dim)
        # import pdb
        # pdb.set_trace()
        attention = torch.matmul(query, enemy_embedding.transpose(1,2))
        attention = attention.view(enemy_raw.shape[0],-1)
        if(use_mask):
            pred, _ = enemy_raw.max(2)
            attn_mask = pred
            one = torch.ones(attn_mask.shape).cuda()
            zero = torch.zeros(attn_mask.shape).cuda()
            attn_mask = where(attn_mask > 0,one,zero)
            attn_mask = attn_mask.byte()
            attn_mask = 1 - attn_mask
            attn_mask = Variable(attn_mask)
            attention = attention.masked_fill(attn_mask, -100)  # 给需要mask的地方设置一个负无穷。
        # attention = F.softmax(attention)
        return attention

class TowerAttention(Model):
    def __init__(self,args,in_dim_global,in_dim_target, query_dim):
        super(TowerAttention, self).__init__(args)
        self.in_dim_global = in_dim_global   #  256
        self.query_dim = query_dim           #  8
        self.in_dim_target = in_dim_target   #  7
        self.FC = nn.Linear(in_dim_global, query_dim)
        self.FC_2 = nn.Linear(self.in_dim_target, self.query_dim)
        self.attn = nn.Linear(self.query_dim * 2, query_dim)
        self.softmax = self.softmax = nn.Softmax()

    def forward(self,x,target_embdding,tower_embedding,tower_raw,use_mask = False):
    #def forward(self,x,tower_embedding,tower_raw,use_mask = False):
        
        #assert enemy_embedding.shape[1] == self.query_dim
        # import pdb
        # pdb.set_trace()
        query = self.FC(x)  # 1 x 8
        target = self.FC_2(target_embdding)  #1 x 7 --> 1 x 8
        attn_weight = self.softmax(
            self.attn(torch.cat((target, query), 1)))
        attn_weight = self.attn(torch.cat((target, query), 1))
        attention = torch.matmul(attn_weight.view(-1,1,self.query_dim),  # 4 x 1 x 8
                                tower_embedding.transpose(1,2))  # 4 x 8 x 20      -- > 4 x 1 x 20
        
        attention = attention.view(attention.data.shape[0],-1)  # 4 x 20
        if (use_mask):
            pred, _ = tower_raw.max(2)
            attn_mask = pred
            one = torch.ones(attn_mask.shape).cuda()
            zero = torch.zeros(attn_mask.shape).cuda()
            attn_mask = where(attn_mask > 0,one,zero)
            attn_mask = attn_mask.byte()
            attn_mask = 1 - attn_mask
            attn_mask = Variable(attn_mask)
            attention = attention.masked_fill(attn_mask, -100)  # 给需要mask的地方设置一个负无穷。
        #attention = F.softmax(attention)
        return attention
    