Wrapper (Python Side)
=====================
Here we introduce the Python interface that interacts with C++-side to retrieve batch of game states. Using the wrapper, training procedure with different RL methods can be written in several lines:
::
    # Initialize game environment.
    GC = [game_env].GameContext([your game params])
    co = [game_env].ContextOptions()

    # Define batch descriptions 
    batch_descriptions = [your batch descriptions]

    # Initialize wrapper.
    GameContext = GCWrapper(GC, co, batch_descriptions)

    # Define callback function.
    def on_actor(input, input_gpu, reply):
        [your code to deal with the actor batch]

    # Register the callback function
    GameContext.reg_callback("actor", on_actor)

    GameContext.Start()
    while True:
        GameContext.Run()

    GameContext.Stop()

In the following, we explain each component in details.

The Batch Protocol
---------------------------
Depending on different models we use, we might need different types of batches in one training procedure. For example: 

* Some batches contain states and action probabililty
* Some contain rewards and terminal signals
* Some require long history. 

To satisfy different needs, we use a dict of descriptions to specify the batch we want. The following shows an example:
::
    desc = {
        "actor": (
            dict(id="", s=str(args.hist_len), last_r="", last_terminal="", _batchsize=str(args.batchsize), _T="1"),
            dict(rv="", pi=str(num_action), V="1", a="1", _batchsize=str(args.batchsize), _T="1")
        ), 
        "optimizer" : (
            dict(rv="", id="", pi=str(num_action), s=str(args.hist_len), a="1", r="1", V="1", seq="", terminal="", _batchsize=str(args.batchsize), _T=str(args.T)),
            None
        )
    }

The descriptions are a dict of tuple of dicts. Each dict entry corresponds to a *collector* that is in charge of a particular type of batch.
ELF will allocate shared memory between C++ threads and Python for each specified batch. When ELF runs, it will wait until any collector 
has got a complete batch and returns the collector id. Note that following this design, we could have multiple actors, which is very useful for self-play, etc.

Each dict entry is a key-value pair ``key, (input_desc, reply_desc)``:

* ``key`` is the name of the collector.
* ``input_desc`` is an *input batch* that contains all variables that have been filled by the blocked game environemnts;
* ``reply_desc`` is a *reply batch* that contains all variables to be filled by the Python side. It is omitted if the description is ``None``. 

Both input and reply batches will be allocated shared memory separately. ELF supports using PyTorch tensor or Numpy array as the shared memory. 
This makes it usable for other DL platform (e.g., Tensorflow) than PyTorch.

Batch Description
------------------
Now let's take a look at the detailed description of one input batch:
::
    dict(s="", pi="", r="", a="", _batchsize="128", _T="2")

This description leads to a batch with batchsize as ``128`` and history length to be ``2``. This means that ``len(batch) = 2`` and the first dimension of all batch element is ``128``. 
Besides, the batch contains the following infomation:
:: 
    batch[0]["s"] = FloatTensor(batchsize, [size that is defined in the wrapper])
    batch[0]["pi"] = FloatTensor(batchsize, [num of action])
    batch[0]["r"] = FloatTensor(batchsize))
    batch[0]["a"] = IntTensor(batchsize)

And since the length of a batch is 2, ``batch[1]`` has the same structure, but is allocated in a separate memory region.

Note that the meaning of each key and types and dimensions of each tensor (except for the first one) is defined in the C++ wrapper. 
Depending on how the C++-wrapper is implemented, you can also send arguments in the description for each key. For example, 
setting ``s="4"`` might mean you are using the last four frames as the input.  

Here is a table of common keys and their meanings:

=============  =========
Key            Meaning
=============  =========
s              Game states
a              Next action
pi             Action probability
V              Value function of the current state
Q              Q function of the current state
A              Advantage function of the current state
r              The reward of leaving the current state with an action in this game environment
last_r         The reward of leaving the last state with an action in this game environment
terminal       Whether the current state is a terminal state
last_terminal  Whether the last state is a terminal state
id             Game environment id
seq            Game environment sequence number. When a new episode starts, its sequence number is 0
_batchsize     batchsize
_T             History length
=============  =========

Register callback function
--------------------------
Once we setup the batches, we then need to register the callback functions for each collector with :func:`reg_callback`. 
The callback function has the signature ``cb(input_batch, input_batch_gpu, reply_batch)``: 
::
    model = [your model]

    def on_actor(input_batch, input_batch_gpu, reply_batch):
        output = model(input_batch_gpu)
        reply_batch[0]["a"][:] = sample_action(output["pi"])
        
    GameContext.reg_callback("actor", on_actor)

Once the callback function is registered, it will be automatically called when a batch of desired key is returned.

Detailed Documents
------------------

.. currentmodule:: elf

.. autoclass:: GCWrapper
    :members:

    .. automethod:: __init__


