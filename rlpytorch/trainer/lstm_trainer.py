import torch
from torch.autograd import Variable

from ..args_provider import ArgsProvider
from ..stats import Stats
from ..utils import HistState
from .utils import ModelSaver, MultiCounter

from datetime import datetime

class LSTMTrainer:
    def __init__(self, verbose=False):
        self.stats = Stats("trainer")
        self.saver = ModelSaver()
        self.counter = MultiCounter()

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("freq_update", 1),
            ],
            more_args = ["num_games", "batchsize", "T", "gpu"],
            on_get_args = self._init,
            child_providers = [ self.saver.args, self.stats.args ],
        )

    def _init(self, _):
        # [TODO] Hard coded now, need to fix.
        num_hiddens = 13 * 25
        def init_state():
            return torch.FloatTensor(num_hiddens).cuda(self.args.gpu).zero_()
        self.hs = HistState(self.args.T, init_state)
        self.stats.reset()

    def episode_start(self, i):
        pass

    def actor(self, batch):
        self.counter.inc("actor")

        ids = batch["id"][0]
        seqs = batch["seq"][0]

        self.hs.preprocess(ids, seqs)
        hiddens = Variable(self.hs.newest(ids, 0))

        m = self.mi["actor"]
        m.set_volatile(True)
        state_curr = m(batch.hist(0), hiddens)
        m.set_volatile(False)

        reply_msg = self.sampler.sample(state_curr)
        reply_msg["rv"] = self.mi["actor"].step

        next_hiddens = m.transition(state_curr["h"], reply_msg["a"])

        self.hs.feed(ids, next_hiddens.data)

        self.stats.feed_batch(batch)
        return reply_msg

    def train(self, batch):
        self.counter.inc("train")
        mi = self.mi

        ids = batch["id"][0]
        T = batch["s"].size(0)

        hiddens = self.hs.newest(ids, T - 1)

        mi.zero_grad()
        self.rl_method.update(mi, batch, hiddens, self.counter.stats)
        mi.update_weights()

        if self.counter.counts["train"] % self.args.freq_update == 0:
            mi.update_model("actor", mi["model"])

    def episode_summary(self, i):
        args = self.args

        prefix = "[%s][%d] Iter" % (str(datetime.now()), args.batchsize) + "[%d]: " % i
        print(prefix)
        if self.counter.counts["train"] > 0:
            self.saver.feed(self.mi["model"])

        print("Command arguments " + str(args.command_line))
        self.counter.summary(global_counter=i)
        print("")

        self.stats.print_summary()
        if self.stats.count_completed() > 10000:
            self.stats.reset()

    def setup(self, rl_method=None, mi=None, sampler=None):
        self.rl_method = rl_method
        self.mi = mi
        self.sampler = sampler


