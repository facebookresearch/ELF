from rlpytorch import Model, ActorCritic
from multiple_prediction import MultiplePrediction
from mcts_prediction import MCTSPrediction

import os
import torch
import torch.nn as nn
import torch.distributed as dist


class Block(Model):
    def __init__(self, args):
        super(Block, self).__init__(args)
        self.dim = getattr(args, "dim", 128)
        self.relu = nn.LeakyReLU(0.1) if not getattr(args, "no_leaky_relu", False) else nn.ReLU()
        self.conv_lower = self._conv_layer()
        self.conv_upper = self._conv_layer(relu=False)

    def _conv_layer(self, input_channel=None, output_channel=None, kernel=3, relu=True):
        if input_channel is None:
            input_channel = self.dim
        if output_channel is None:
            output_channel = self.dim

        layers = []
        layers.append(nn.Conv2d(input_channel, output_channel, kernel, padding=kernel // 2))
        if not getattr(self.args, "no_bn", False):
            layers.append(nn.BatchNorm2d(output_channel))
        if relu:
            layers.append(self.relu)

        return nn.Sequential(*layers)

    def forward(self, s):
        s1 = self.conv_lower(s)
        s1 = self.conv_upper(s1)
        s1 = s1 + s
        s = self.relu(s1)
        return s


class GoResNet(Model):
    def __init__(self, args):
        super(GoResNet, self).__init__(args)
        self.blocks = []
        for _ in range(args.num_block):
            self.blocks.append(Block(args))
        self.resnet = nn.Sequential(*self.blocks)

    def forward(self, s):
        return self.resnet(s)


class Model_PolicyValue(Model):
    def __init__(self, args):
        super(Model_PolicyValue, self).__init__(args)

        self.args = args
        params = args.params

        self.board_size = params["board_size"]
        self.num_future_actions = params["num_future_actions"]
        self.num_planes = params["num_planes"]
        # print("#future_action: " + str(self.num_future_actions))
        # print("#num_planes: " + str(self.num_planes))

        # Network structure of AlphaGo Zero
        # https://www.nature.com/nature/journal/v550/n7676/full/nature24270.html

        # Simple method. multiple conv layers.
        self.relu = nn.LeakyReLU(0.1) if not getattr(args, "no_leaky_relu", False) else nn.ReLU()
        self.dim = getattr(args, "dim", 128)
        last_planes = self.num_planes

        self.init_conv = self._conv_layer(last_planes)

        self.pi_final_conv = self._conv_layer(self.dim, 2, 1)
        self.value_final_conv = self._conv_layer(self.dim, 1, 1)

        d = self.board_size ** 2

        # Plus 1 for pass.
        self.pi_linear = nn.Linear(d * 2, d + 1)
        self.value_linear1 = nn.Linear(d, 256)
        self.value_linear2 = nn.Linear(256, 1)

        # Softmax as the final layer
        self.logsoftmax = nn.LogSoftmax(dim=1)
        self.tanh = nn.Tanh()
        self.resnet = GoResNet(args)
        if args.use_data_parallel:
            self.resnet = nn.DataParallel(self.resnet, output_device=getattr(args, "gpu", 0))
        self._check_and_init_distributed_model()

    def _check_and_init_distributed_model(self):
        '''
        if not self.args.use_data_parallel_distributed:
            return

        if not dist.is_initialized():
            world_size = getattr(self.args, "dist_world_size")
            url = getattr(self.args, "dist_url")
            rank = getattr(self.args, "dist_rank")
            # This is for SLURM's special use case
            if rank == -1:
                rank = int(os.environ.get("SLURM_NODEID"))

            print("=> Distributed training: world size: {}, rank: {}, URL: {}".
                    format(world_size, rank, url))

            dist.init_process_group(backend="nccl",
                                    init_method=url,
                                    rank=rank,
                                    world_size=world_size)

        # Initialize the distributed data parallel model
        master_gpu = getattr(self.args, "gpu")
        if master_gpu is None:
            raise RuntimeError("Distributed training requires "
                               "to put the model on the GPU, but the GPU is "
                               "not given in the arguement")
        # This is needed for distributed model since the distributed model
        # initialization will require the model be on the GPU, even though
        # the later code will put the same model on the GPU again with
        # args.gpu, so this should be ok
        self.resnet.cuda(master_gpu)
        self.resnet = nn.parallel.DistributedDataParallel(
                self.resnet,
                output_device=master_gpu)'''
        pass


    def _conv_layer(self, input_channel=None, output_channel=None, kernel=3, relu=True):
        if input_channel is None:
            input_channel = self.dim
        if output_channel is None:
            output_channel = self.dim

        layers = []
        layers.append(nn.Conv2d(input_channel, output_channel, kernel, padding=kernel // 2))
        if not getattr(self.args, "no_bn", False):
            layers.append(nn.BatchNorm2d(output_channel))
        if relu:
            layers.append(self.relu)

        return nn.Sequential(*layers)

    def get_define_args():
        return [
            ("no_bn", dict(action="store_true")),
            ("no_leaky_relu", dict(action="store_true")),
            ("num_block", 20),
            ("use_data_parallel", dict(action="store_true")),
            ("use_data_parallel_distributed", dict(action="store_true")),
            ("dim", 128),
            ("dist_rank", -1),
            ("dist_world_size", -1),
            ("dist_url", ""),
        ]

    def forward(self, x):
        s = self._var(x["s"])

        s = self.init_conv(s)
        s = self.resnet(s)

        d = self.board_size ** 2

        pi = self.pi_final_conv(s)
        pi = self.pi_linear(pi.view(-1, d * 2))
        logpi = self.logsoftmax(pi)
        pi = logpi.exp()

        V = self.value_final_conv(s)
        V = self.relu(self.value_linear1(V.view(-1, d)))
        V = self.value_linear2(V)
        V = self.tanh(V)

        return dict(logpi=logpi, pi=pi, V=V)

# Format: key, [model, method]
Models = {
   "df_pred" : [Model_PolicyValue, MultiplePrediction],
   "df_kl" : [Model_PolicyValue, MCTSPrediction]
}
