import torch
import torch.nn as nn
import math
from .args_utils import ArgsProvider

class PolicyGradient:
    def __init__(self, args=None):
        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("entropy_ratio", dict(type=float, help="The entropy ratio we put on PG", default=0.01)),
                ("grad_clip_norm", dict(type=float, help="Gradient norm clipping", default=None)),
                ("discount", dict(type=float, default=0.99)),
                ("min_prob", dict(type=float, help="Minimal probability used in training", default=1e-6)),
                ("ratio_clamp", 10),
            ]
            on_get_args = self._init,
            fixed_args = args,
        )

    def _init(self, args):
        self.policy_loss = nn.NLLLoss().cuda()
        self.value_loss = nn.SmoothL1Loss().cuda()

        grad_clip_norm = getattr(args, "grad_clip_norm", None)
        self.policy_gradient_weights = None

        def _policy_backward(layer, grad_input, grad_output):
            if self.policy_gradient_weights is None: return
            # Multiply gradient weights

            # This works only on pytorch 0.2.0
            grad_input[0].data.mul_(self.policy_gradient_weights.view(-1, 1))
            if grad_clip_norm is not None:
                average_norm_clip(grad_input[0], grad_clip_norm)

        def _value_backward(layer, grad_input, grad_output):
            if grad_clip_norm is not None:
                average_norm_clip(grad_input[0], grad_clip_norm)

        # Backward hook for training.
        self.policy_loss.register_backward_hook(_policy_backward)
        self.value_loss.register_backward_hook(_value_backward)

    def _compute_one_policy_entropy_err(self, pi, a):
        batchsize = a.size(0)

        # Add normalization constant
        logpi = (pi + self.args.min_prob).log()

        # Get policy. N * #num_actions
        policy_err = self.policy_loss(logpi, a)
        entropy_err = (logpi * pi).sum() / batchsize
        return dict(policy_err=policy_err, entropy_err=entropy_err)

    def _compute_policy_entropy_err(self, pi, a):
        args = self.args

        errs = { }
        if isinstance(pi, list):
            # Action map, and we need compute the error one by one.
            for i, pix in enumerate(pi):
                for j, pixy in enumerate(pix):
                    errs = accumulate(errs, self._compute_one_policy_entropy_err(pixy, a[:,i,j], args.min_prob))
        else:
            errs = self._compute_one_policy_entropy_err(pi, a)

        return errs

    def feed(self, batch, stats):
        ''' Rquired keys:
                V: value,
                R: cumulative rewards,
                a: action
                pi: policy
                old_pi (optional): old policy (in order to get importance factor)
            All inputs are of type torch.autograd.Variable.
        '''
        args = self.args
        R = batch["R"]
        V = batch["V"]
        a = batch["a"]
        pi = batch["pi"]

        # We need to set it beforehand.
        # Note that the samples we collect might be off-policy, so we need
        # to do importance sampling.
        self.policy_gradient_weights = R.data - V.data

        if "old_pi" in batch:
            old_pi = batch["old_pi"].data
            # Cap it.
            coeff = torch.clamp(pi.data.div(old_pi), max=args.ratio_clamp).gather(1, a.view(-1, 1)).squeeze()
            self.policy_gradient_weights.mul_(coeff)
            # There is another term (to compensate clamping), but we omit it for now.

        # Compute policy gradient error:
        errs = self._compute_policy_entropy_err(pi, a)
        policy_err = errs["policy_err"]
        entropy_err = errs["entropy_err"]

        # Compute critic error
        value_err = self.value_loss(V, R)
        overall_err = policy_err + entropy_err * args.entropy_ratio + value_err

        if stats is not None:
            stats["value_err"].feed(value_err.data[0])
            stats["policy_err"].feed(policy_err.data[0])
            stats["entropy_err"].feed(entropy_err.data[0])
            stats["rms_advantage"].feed(self.policy_gradient_weights.norm() / math.sqrt(batchsize))

        return overall_err

