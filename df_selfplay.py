# Console for DarkForest
import sys
import os
from rlpytorch import load_env, Evaluator, ModelInterface, ArgsProvider, EvalIters

if __name__ == '__main__':
    evaluator = Evaluator(stats=False)
    # Set game to online model.
    env, args = load_env(os.environ, evaluator=evaluator, overrides=dict(mode="selfplay", T=1))

    GC = env["game"].initialize()
    model = env["model_loaders"][0].load_model(GC.params)
    mi = ModelInterface()
    mi.add_model("model", model)
    mi.add_model("actor", model, copy=True, cuda=args.gpu is not None, gpu_id=args.gpu)
    mi["model"].eval()
    mi["actor"].eval()

    evaluator.setup(mi=mi)

    def actor(batch):
        reply = evaluator.actor(batch)
        # import pdb
        # pdb.set_trace()
        return reply

    GC.reg_callback_if_exists("actor", actor)

    GC.Start()

    evaluator.episode_start(0)

    while True:
        GC.Run()

    GC.Stop()
