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
    --num_games 1024 --batchsize 128              # Set number of games to be 1028 and batchsize to be 128.  
    --freq_update 50                              # Update behavior policy after 50 updates of the model.
    --fs_opponent 20                              # How often your opponent makes a decision (every 20 ticks)
    --latest_start 500  --latest_start_decay 0.99 # Use rule-based AI for the first 500 ticks, then trained AI takes over. latest_start decays with rate latest_start_decay. 
    --opponent_type AI_SIMPLE                     # Use AI_SIMPLE as rule-based AI
    --tqdm                                        # Show progress bar.
```

You can control the number of CPUs used in the training using `taskset -c`. The following is a sample output:
```
$ game=../rts/game_MC/game model=actor_critic model_file=../rts/game_MC/model taskset -c 0-9 python3 run.py --batchsize 128 --freq_update 50 --fs_opponent 20 --latest_start 500 --latest_start_decay 0.99 --num_games 1024 --opponent_type AI_SIMPLE --tqdm
Namespace(T=6, actor_only=False, ai_type='AI_NN', batchsize=128, discount=0.99, entropy_ratio=0.01, epsilon=0.0, eval=False, freq_update=50, fs_ai=50, fs_opponent=20, game_multi=None, gpu=None, grad_clip_norm=None, greedy=False, handicap_level=0, latest_start=500, latest_start_decay=0.99, load=None, max_tick=30000, mcts_threads=64, min_prob=1e-06, num_episode=10000, num_games=1024, num_minibatch=5000, opponent_type='AI_SIMPLE', ratio_change=0, record_dir='./record', sample_node='pi', sample_policy='epsilon-greedy', save_dir=None, save_prefix='save', seed=0, simple_ratio=-1, tqdm=True, verbose_collector=False, verbose_comm=False, wait_per_group=False)
Version:  bf1304010f9609b2114a1adff4aa2eb338695b9d_staged
Num Actions:  9
Num unittype:  6
100%|█████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████| 5000/5000 [01:35<00:00, 52.37it/s]
[2017-07-12 09:04:13.212017][128] Iter[0]:
Train count: 820/5000, actor count: 4180/5000
Save to ./
Filename = ./save-820.bin
Command arguments run.py --batchsize 128 --freq_update 50 --fs_opponent 20 --latest_start 500 --latest_start_decay 0.99 --num_games 1024 --opponent_type AI_SIMPLE --tqdm
0:acc_reward[4100]: avg: -0.34079, min: -0.58232[1580], max: 0.25949[185]
0:cost[4100]: avg: 2.15912, min: 1.97886[2140], max: 2.31487[1173]
0:entropy_err[4100]: avg: -2.13493, min: -2.17945[438], max: -2.04809[1467]
0:init_reward[820]: avg: -0.34093, min: -0.56980[315], max: 0.26211[37]
0:policy_err[4100]: avg: 2.16714, min: 1.98384[1520], max: 2.31068[1176]
0:predict_reward[4100]: avg: -0.33676, min: -1.36083[1588], max: 0.39551[195]
0:reward[4100]: avg: -0.01153, min: -0.13281[1109], max: 0.04688[124]
0:rms_advantage[4100]: avg: 0.15646, min: 0.02189[800], max: 0.79827[564]
0:value_err[4100]: avg: 0.01333, min: 0.00024[800], max: 0.06569[1549]

 86%|████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████▉                    | 4287/5000 [01:23<00:15, 46.97it/s]
```

To evaluate a model for MiniRTS, try the following command:
```bash
eval_only=1 game=../rts/game_MC/game model=actor_critic model_file=../rts/game_MC/model \ 
python3 run.py 
    --batchsize 128 
    --fs_opponent 20
    --latest_start 500 
    --latest_start_decay 0.99 
    --num_games 1024 
    --opponent_type AI_SIMPLE
    --stats winrate
    --num_eval 10000
    --tqdm
```


