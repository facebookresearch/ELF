# RLPyTorch: A Simple Reinforcement Learning Package in PyTorch

Overview    
==============
Here we provide a simple reinforcement learning package as a backend for ELF.  


Actor Critic model  
-------------
We implemented advantagous actor-critic models, similar to Vanilla A3C[cite], but with off-policy corrections with importance sampling. 

Specifically, we use the trajectory sampled from the previous version of the actor in the actor-critic update. 

