RLPyTorch
=================

Design principle
------------------
In RLPytorch, models/algorithms communicate with each other via Python dictionary. Each model/algorithm takes the key they want, and return the key-pair for others to use. For example:

* A model might take a ``dict(s=states)`` and returns ``dict(pi=policy_distribution, V=values)``.
* An actor-critic algorithm might take a ``dict(s=states, a=actions, r=rewards)`` and run forward/backward updates.

Note that each entry in the dict is a PyTorch tensor whose first dimension must be the batchsize. The remaining dimensions depend on the semantic meaning of the key. For example:

* ``input["s"]`` encodes the current states of a batch of games. For Atari games, ``input["s"]`` can be ``128x3x100x80``, which encodes the current RGB frame of size ```100x80`` for 128 games. If 4 history frames are concatenated, the dimensions of ``input["s"]`` may become ``128x12x100x80``.

* ``output["pi"]`` encodes the action probability of a batch of games. ``output["pi"]`` is ``128x6``, which encodes the probability among 6 actions for 128 games.

Architecture
------------

.. currentmodule:: rlpytorch

.. autoclass:: Model
    :members:

    .. automethod:: __init__
.. autoclass:: ModelInterface
    :members:

    .. automethod:: __init__
.. autoclass:: ModelLoader
    :members:

    .. automethod:: __init__

Example: Actor-Critic in 30 lines
---------------------------------
Here is an example of an actor-critic algorithm::

    # A3C
    def update(self, batch):
        ''' Actor critic model '''
        bt = batch[self.args.T - 1]

        R = deepcopy(bt["V"])
        batchsize = R.size(0)
        R.resize_(batchsize, 1)

        for i, terminal in enumerate(bt["terminal"]):
            if terminal: R[i] = bt["r"][i]

        for t in range(self.args.T - 2, -1, -1):
            bt = batch[t]

            # Forward pass
            state_curr = self.model_interface.forward("model", bt)

            # Compute the reward.
            R = R * self.args.discount + bt["r"]
            # If we see any terminal signal, do not backprop
            for i, terminal in enumerate(bt["terminal"]):
                if terminal: R[i] = state_curr["V"].data[i]

            # We need to set it beforehand.
            self.policy_gradient_weights = R - state_curr["V"].data

            # Compute policy gradient error:
            errs = self._compute_policy_entropy_err(state_curr["pi"], bt["a"])
            # Compute critic error
            value_err = self.value_loss(state_curr["V"], Variable(R))

            overall_err = value_err + errs["policy_err"]
            overall_err += errs["entropy_err"] * self.args.entropy_ratio
            overall_err.backward()
