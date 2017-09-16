# Console for DarkForest
from rlpytorch import load_env, Evaluator, ModelInterface, ArgsProvider, EvalIters

class DFConsole:
    def __init__(self):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
            ]
        )

    def main_loop(self):
        evaluator = Evaluator()
        # Set game to online model.
        env, args = load_env(os.environ, evaluator=evaluator, overrides=dict(num_games=1, batchsize=1))

        GC = env["game"].initialize()
        model = env["model_loaders"][0].load_model(GC.params)
        mi = ModelInterface()
        mi.add_model("model", model, optim_params={ "lr" : 0.001})
        mi.add_model("actor", model, copy=True, cuda=True, gpu_id=args.gpu)

        def actor(batch):
            reply = evaluator.actor(batch)
            return reply

        evaluator.setup(sampler=env["sampler"], mi=mi)

        GC.reg_callback("actor", actor)
        GC.Start()

        evaluator.episode_start(0)

        while True:
            #
            GC.Run()
        GC.Stop()


