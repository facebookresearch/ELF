import torch
import torch.nn as nn

# Some utility functions
def average_norm_clip(grad, clip_val):
    ''' The first dimension will be batchsize '''
    batchsize = grad.size(0)
    avg_l2_norm = 0.0
    for i in range(batchsize):
        avg_l2_norm += grad[i].data.norm()
    avg_l2_norm /= batchsize
    if avg_l2_norm > clip_val:
        # print("l2_norm: %.5f clipped to %.5f" % (avg_l2_norm, clip_val))
        grad *= clip_val / avg_l2_norm

def accumulate(acc, new):
    ret = { k: new[k] if a is None else a + new[k] for k, a in acc.items() if k in new }
    ret.update({ k : v for k, v in new.items() if not (k in acc) })
    return ret

def add_err(overall_err, new_err):
    if overall_err is None:
        return new_err
    else:
        overall_err += new_err
        return overall_err

def add_stats(stats, key, value):
    if stats:
        stats[key].feed(value)

def check_terminals(has_terminal, batch):
    # Block backpropagation if we go pass a terminal node.
    for i, terminal in enumerate(batch["terminal"]):
        if terminal: has_terminal[i] = True

def check_terminals_anyT(has_terminal, batch, T):
    for t in range(T):
        check_terminals(has_terminal, batch[t])


