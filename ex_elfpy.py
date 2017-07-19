from elf_python import GCWrapper, Simulator
import torch
import random
import tqdm

class SimpleGame(Simulator):
    MAX_STATE = 6
    def __init__(self, id, desc):
        super(SimpleGame, self).__init__(id, desc)

    def restart(self):
        # print("[%s] Game restarts. game_counter = %d, seq = %d" % (self.agent_name, self.game_counter, self.seq))
        self.state = 0

    @property
    def terminal(self):
        # print("[%s] Game end. game_counter = %d, seq = %d" % (self.agent_name, self.game_counter, self.seq))
        return self.state == SimpleGame.MAX_STATE

    def get_key(self, key):
        if key == "s":
            # print("[%s] Getting state, state = %d" % (self.agent_name, self.state))
            return self.state
        elif key == "r":
            reward = 1 if self.state == 4 else 0
            # print("[%s] Getting reward, reward = %d" % (self.agent_name, reward))
            return reward
        else:
            raise ValueError("Unknown key (in get_key): " + key)

    def set_key(self, key, value):
        if key == "a":
            # Get next action, proceed!
            # print("[%s] receiving action = %d" % (self.agent_name, value))
            self._forward(value)
        else:
            raise ValueError("Unknown key (in set_key): " + key + " value: " + str(value))

    def _forward(self, action):
        if action == 0:
            self.state += 1
            if self.state >= SimpleGame.MAX_STATE:
                self.state = SimpleGame.MAX_STATE
        elif action == 1:
            self.state -= 1
            if self.state < 0:
                self.state = 0

class Trainer:
    def __init__(self):
        pass

    def train(self, cpu_batch, gpu_batch):
        pass

    def actor(self, cpu_batch, gpu_batch):
        # Random
        batchsize = cpu_batch[0]["s"].size(0)
        action = torch.IntTensor(batchsize)
        for i in range(batchsize):
            action[i] = random.randint(0, 1)
        return dict(a = action)

if __name__ == '__main__':
    desc = dict(
        actor=dict(
            input = dict(s=""),
            reply = dict(a=""),
            connector = "actor-connector"
        ),
        train=dict(
            input = dict(s="", r=""),
            reply = None,
            connector = "trainer-connector"
        )
    )
    wrapper = GCWrapper(SimpleGame, desc, 32, 8, 1)
    trainer = Trainer()
    wrapper.reg_callback("train", trainer.train)
    wrapper.reg_callback("actor", trainer.actor)

    for i in tqdm.trange(1000):
        wrapper.Run()

    print("Done")
