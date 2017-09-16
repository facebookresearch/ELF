# Console for DarkForest
from rlpytorch import load_env, Evaluator, ModelInterface, ArgsProvider, EvalIters

def move_parse(v):
    x = ord(v[0].lower()) - ord('a')
    # Skip 'i'
    if x >= 9: x -= 1
    y = int(v[1:])
    return x, y

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
        env, args = load_env(os.environ, evaluator=evaluator, overrides=dict(num_games=1, batchsize=1, online=True, list_file="", greedy=True))

        GC = env["game"].initialize()
        model = env["model_loaders"][0].load_model(GC.params)
        mi = ModelInterface()
        mi.add_model("model", model, optim_params={ "lr" : 0.001})
        mi.add_model("actor", model, copy=True, cuda=True, gpu_id=args.gpu)

        def actor(batch):
            print(batch.GC.ShowBoard(0))
            # Ask user to choose
            while True:
                cmd = input("DF> ")
                items = cmd.split()
                if items[0] == 'p':
                    x, y = move_parse(items[1])
                    return dict(a=[x * 19 + y])
                elif items[0] == 'c':
                    return evaluator.actor(batch)
                else:
                    print("Invalid input: " + cmd + ". Please try again")

        evaluator.setup(sampler=env["sampler"], mi=mi)

        GC.reg_callback("actor", actor)
        GC.Start()

        evaluator.episode_start(0)

        while True:
            GC.Run()
        GC.Stop()

if __name__ == '__main__':
    console = DFConsole()
    console.main_loop()
