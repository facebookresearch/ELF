from ..args_provider import ArgsProvider
from ..stats import Stats
import tqdm

class EvalIters:
    def __init__(self):
        self.stats = Stats("eval")
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("num_eval", 500),
                ("tqdm", dict(action="store_true"))
            ],
            on_get_args = self._on_get_args,
            child_providers = [ self.stats.args ]
        )

    def _on_get_args(self, _):
        self.stats.reset()

    def iters(self):
        if self.args.tqdm:
            import tqdm
            tq = tqdm.tqdm(total=self.args.num_eval, ncols=50)
            while self.stats.count_completed() < self.args.num_eval:
                old_n = self.stats.count_completed()
                yield old_n
                diff = self.stats.count_completed() - old_n
                tq.update(diff)
            tq.close()
        else:
            while self.stats.count_completed() < self.args.num_eval:
                yield self.stats.count_completed()

        self.stats.print_summary()


