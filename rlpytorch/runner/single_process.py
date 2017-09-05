from ..args_provider import ArgsProvider
import tqdm

class SingleProcessRun:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("num_minibatch", 5000),
                ("num_episode", 10000),
                ("tqdm", dict(action="store_true")),
            ]
        )

    def setup(self, GC, episode_start=None, episode_summary=None):
        self.GC = GC
        self.episode_summary = episode_summary
        self.episode_start = episode_start

    def run(self):
        self.GC.Start()
        args = self.args
        for k in range(args.num_episode):
            if self.episode_start is not None:
                self.episode_start(k)
            if args.tqdm: iterator = tqdm.trange(args.num_minibatch, ncols=50)
            else: iterator = range(args.num_minibatch)

            for i in iterator:
                self.GC.Run()

            if self.episode_summary is not None:
                self.episode_summary(k)

        self.GC.PrintSummary()
        self.GC.Stop()

    def run_multithread(self):
        def train_thread():
            args = self.args
            for i in range(args.num_episode):
                for k in range(args.num_minibatch):
                    if self.episode_start is not None:
                        self.episode_start(k)

                    if k % 500 == 0:
                        print("Receive minibatch %d/%d" % (k, args.num_minibatch))

                    self.GC.RunGroup("train")

                # Print something.
                self.episode_summary(i)

        def actor_thread():
            while True:
                self.GC.RunGroup("actor")

        self.GC.Start()

        # Start the two threads.
        train_th = threading.Thread(target=train_thread)
        actor_th = threading.Thread(target=actor_thread)
        train_th.start()
        actor_th.start()
        train_th.join()
        actor_th.join()
