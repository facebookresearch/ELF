# Console for DarkForest
import sys
import os
from rlpytorch import load_env, Evaluator, ModelInterface, ArgsProvider, SingleProcessRun

if __name__ == '__main__':
    evaluator = Evaluator(stats=None)
    runner = SingleProcessRun()
    # Set game to online model.
    env, args = load_env(os.environ, evaluator=evaluator, runner=runner, overrides=dict(T=1))

    GC = env["game"].initialize()
    model = env["model_loaders"][0].load_model(GC.params)
    mi = ModelInterface()
    mi.add_model("actor", model, copy=True, cuda=args.gpu is not None, gpu_id=args.gpu)
    mi["actor"].eval()

    evaluator.setup(mi=mi)

    total_batchsize = 0
    total_sel_batchsize = 0

    def actor(batch):
        global total_batchsize, total_sel_batchsize
        reply = evaluator.actor(batch)
        total_sel_batchsize += batch.batchsize
        total_batchsize += batch.max_batchsize

        if total_sel_batchsize >= 500000:
            print("Batch usage: %d/%d (%.2f%%)" %
                  (total_sel_batchsize, total_batchsize, 100.0 * total_sel_batchsize / total_batchsize))
            total_sel_batchsize = 0
            total_batchsize = 0

        # eval_iters.stats.feed_batch(batch)
        return reply

    GC.reg_callback_if_exists("actor", actor)

    runner.setup(GC, episode_summary=evaluator.episode_summary,
                episode_start=evaluator.episode_start)

    runner.run()

    '''
    GC.Start()

    i = 0
    while True:
        evaluator.episode_start(i)
        for n in eval_iters.iters():
            GC.Run()
        evaluator.episode_summary(i)
        i += 1

    GC.Stop()
    '''
