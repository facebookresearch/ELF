# ELF: An Extensive, Lightweight and Flexible Platform for Game Research

Overview
===============

ELF is an **E**xtensive, **L**ightweight and **F**lexible platform for game research, in particular for real-time strategy (RTS) games. On the C++-side, ELF hosts multiple games in parallel with C++ threading. On the Python side, ELF returns one batch of game state at a time, making it very friendly for modern RL. In comparison, other platforms (e.g., OpenAI Gym) wraps one single game instance with one Python interface. This makes concurrent game execution a bit complicated, which is a requirement of many modern reinforcement learning algorithms.

For research on RTS games, ELF comes with an fast RTS engine, and three concrete environments: MiniRTS, Capture the Flag and Tower Defense. MiniRTS has all the key dynamics of a real-time strategy game, including gathering resources, building facilities and troops, scouting the unknown territories outside the perceivable regions, and defend/attack the enemy. User can access its internal representation and can freely change the game setting.

![Overview](./overview.png)

ELF has the following characteristics:

* End-to-End: ELF offers an end-to-end solution to game research. It provides miniature real-time strategy game environments, concurrent simulation, intuitive APIs, web-based visualzation, and also comes with a reinforcement learning backend empowered by [Pytorch](https://github.com/pytorch/pytorch) with minimal resource requirement.  

* Extensive: Any game with C/C++ interface can be plugged into this framework by writing a simple wrapper. As an example, we already incorporate Atari games into our framework and show that the simulation speed per core is comparable with single-core version, and is thus much faster than implementation using either multiprocessing or Python multithreading. In the future, we plan to incorporate more environments, e.g., DarkForest Go engine.

* Lightweight: ELF runs very fast with minimal overhead. ELF with a simple game (MiniRTS) built on RTS engine runs **40K frame per second per core** on a MacBook Pro. Training a model from scratch to play MiniRTS **takes a day on 6 CPU + 1 GPU**.  

* Flexible: Pairing between environments and actors is very flexible, e.g., one environment with one agent (e.g., Vanilla A3C), one environment with multiple agents (e.g., Self-play/MCTS), or multiple environment with one actor (e.g., BatchA3C, GA3C). Also, any game built on top of the RTS engine offers full access to its internal representation and dynamics. Besides efficient simulators, we also provide a lightweight yet powerful Reinforcement Learning framework. This framework can host most existing RL algorithms. In this open source release, we have provided state-of-the-art actor-critic algorithms, written in [PyTorch](https://github.com/pytorch/pytorch).

Code Hierarchy  
=================

ELF is organized as follows.
![Hierarchy of ELF](hierarchy.png)

* The folder `elf` contains the game-independent codebase to handle concurrent simulation.
* The folder `atari` contains the wrapper and model for Atari games ([ALE](https://github.com/mgbellemare/Arcade-Learning-Environment) is required).  
* The folder `rts/engine` contains the RTS engine. `rts/game_MC`, `rts/game_CF` and `rts/game_TD` are the three games built on top of the engine.  

Basic Usage  
==============
The pseudo code of ELF is the following.

The initialization looks like the following:

```python

# We run 1024 games concurrently.
num_games = 1024

# Wait for a batch of 256 games.
batchsize = 256  

# The return states contain key 's', 'r' and 'terminal'
# The reply contains key 'a' to be filled from the Python side.
# The definitions of the keys are in the wrapper of the game.  
input_spec = dict(s='', r='', terminal='')
reply_spec = dict(a='')

context = Init(num_games, batchsize, input_spec, reply_spec)
```

The main loop is also very simple:

```python
# Start all game threads and enter main loop.
context.Start()  
while True:
    # Wait for a batch of game states to be ready
    # These games will be blocked, waiting for replies.
    batch = context.Wait()

    # Apply a model to the game state. The output has key 'pi'
    # You can do whatever you want here. E.g., applying your favorite RL algorithms.
    output = model(batch)

    # Sample from the output to get the actions of this batch.
    reply['a'][:] = SampleFromDistribution(output)

    # Resume games.
    context.Steps()   

# Stop all game threads.
context.Stop()  
```

Dependency    
===============

C++ compiler with C++11 support (e.g., gcc >= 4.9) is required. The following libraries are required:
```
tbb
tqdm
```

Reference  
=============

When you use ELF, please reference the associated arXiv [paper](https://arxiv.org/abs/1707.01067).
