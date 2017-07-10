# RLPyTorch: Reinforcement Learning Package in PyTorch

Overview    
==============

Actor Critic model  
-------------
We implemented advantagous actor-critic models, similar to Vanilla A3C[cite], but with off-policy corrections with importance sampling. 

Specifically, we use the trajectory sampled from the previous version of the actor in the actor-critic update. 

How to train    
===============
To train a model for MiniRTS, run the following in the current directory:

```bash
game=../rts/game_MC/game model=actor_critic model_file=../rts/game_MC/model \ 
python3 run.py 
    -—batchsize 128 
    —-freq_update 50 
    —-fs_opponent 20
    -—latest_start 500 
    -—latest_start_decay 0.99 
    —-num_games 1024 
    --opponent_type AI_SIMPLE
```

To evaluate a model for MiniRTS, try the following command:
```bash
eval_only=1 game=../rts/game_MC/game model=actor_critic model_file=../rts/game_MC/model \ 
python3 run.py 
    -—batchsize 128 
    —-fs_opponent 20
    -—latest_start 500 
    -—latest_start_decay 0.99 
    —-num_games 1024 
    --opponent_type AI_SIMPLE
    --stats winrate
    --num_eval 10000
```


