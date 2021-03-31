import torch
import sys

class TD_Sampler:
    def __init__(self,epsilon):
        self.epsilon = epsilon

    def sample_target(self,attention):
        probs = attention
        #print(probs)
        num_action = probs.size(1)
        actions = probs.multinomial(1)[:, 0]  # 按照概率选择一个动作
        return actions

    # def sample_with_check(self,probs):
    #     num_action = probs.size(1)
    #     while True:
    #         actions = probs.multinomial(1)[:, 0]  # 按照概率选择一个动作
    #         cond1 = (actions < 0).sum()
    #         cond2 = (actions >= num_action).sum()
    #         if cond1 == 0 and cond2 == 0:
    #             return actions
    #         print("Warning! sampling out of bound! cond1 = %d, cond2 = %d" % (cond1, cond2))
    #         print("prob = ")
    #         print(probs)
    #         print("action = ")
    #         print(actions)
    #         print("condition1 = ")
    #         print(actions < 0)
    #         print("condition2 = ")
    #         print(actions >= num_action)
    #         print("#actions = ")
    #         print(num_action)
    #         sys.stdout.flush()

    def sample_with_eps(self,probs):
        # import pdb
        # pdb.set_trace()
        actions = self.sample_target(probs)
        #print("Actions Init: ",actions)
        epsilon = self.epsilon

        if epsilon > 1e-10:
            num_action = probs.size(1)
            batchsize = probs.size(0)

            probs = probs.data if isinstance(probs, torch.autograd.Variable) else probs

            rej_p = probs.new().resize_(2)
            rej_p[0] = 1 - epsilon
            rej_p[1] = epsilon
            # rej 按照概率取 0 或 1（batchsize次），取到1时(epsilon)表示此次不选择该动作并随机取样
            rej = rej_p.multinomial(batchsize, replacement=True).byte()
            # 随机取样
            uniform_p = probs.new().resize_(num_action).fill_(1.0 / num_action)
            uniform_sampling = uniform_p.multinomial(batchsize, replacement=True)
            actions[rej] = uniform_sampling[rej]
        return actions
