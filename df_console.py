# Console for DarkForest
import sys
import os
from rlpytorch import load_env, Evaluator, ModelInterface, ArgsProvider, EvalIters

def move2xy(v):
    x = ord(v[0].lower()) - ord('a')
    # Skip 'i'
    if x >= 9: x -= 1
    y = int(v[1:]) - 1
    return x, y

def move2action(v):
    x, y = move2xy(v)
    return x * 19 + y

def xy2move(x, y):
    if x >= 8: x += 1
    return chr(x + 65) + str(y + 1)

def action2move(a):
    x = a // 19
    y = a % 19
    return xy2move(x, y)

class DFConsole:
    def __init__(self):
        self.exit = False

    def prompt(self, prompt_str, batch, evaluator):
        print(batch.GC.ShowBoard(0))
        # Ask user to choose
        while True:
            cmd = input(prompt_str)
            items = cmd.split()
            if items[0] == 'p':
                return dict(a=move2action(items[1]))
            elif items[0] == 'c':
                return evaluator.actor(batch)
            elif items[0] == "a":
                reply = evaluator.actor(batch)
                if "pi" in reply:
                    score, indices = reply["pi"].squeeze().sort(dim=0, descending=True)
                    first_n = int(items[1])
                    for i in range(first_n):
                        print("%s: %.3f" % (action2move(indices[i]), score[i]))
                else:
                    print("No key \"pi\"")
            elif items[0] == 'offline_a':
                if "offline_a" in batch:
                    for i, offline_a in enumerate(batch["offline_a"][0][0]):
                        print("[%d]: %s" % (i, action2move(offline_a)))
                else:
                    print("No offline_a available!")
            elif items[0] == "exit":
                self.exit = True
                return
            else:
                print("Invalid input: " + cmd + ". Please try again")


    def main_loop(self):
        evaluator = Evaluator(stats=False)
        # Set game to online model.
        env, args = load_env(os.environ, evaluator=evaluator, overrides=dict(num_games=1, batchsize=1, greedy=True, T=1))

        GC = env["game"].initialize()
        model = env["model_loaders"][0].load_model(GC.params)
        mi = ModelInterface()
        model.eval()
        mi.add_model("model", model, optim_params={ "lr" : 0.001})
        mi.add_model("actor", model, copy=True, cuda=True, gpu_id=args.gpu)

        def actor(batch):
            return self.prompt("DF> ", batch, evaluator)

        def train(batch):
            self.prompt("DF Train> ", batch, evaluator)

        evaluator.setup(sampler=env["sampler"], mi=mi)

        GC.reg_callback("actor", actor)
        GC.reg_callback("train", train)
        GC.Start()

        evaluator.episode_start(0)

        while True:
            GC.Run()
            if self.exit: break
        GC.Stop()

if __name__ == '__main__':
    console = DFConsole()
    console.main_loop()
