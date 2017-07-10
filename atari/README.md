## Atari Game

Simulator  
=================
The simulator is similar to OpenAI Gym "-v0" setting, in which we apply the same action for k frames, where k is sampled uniformly in {2, 3, 4}.


Compilation  
===================
Please first install Arcade Learning Environment ([ALE](https://github.com/mgbellemare/Arcade-Learning-Environment)). 

After that, just `make` then run `python game.py --rom_file [your game rom file]` to test whether you can run the environment alone. You can put the game rom file in this directory. We do not provide you with the game rom file. 

If you have both python2 and python3 installed, `PYTHON_CONFIG=[your python config dir]/python3.5-config make` might be used for compilation. For now, the interface only works for python3.

Performance     
===============
You should be able to get 650 mean/864 max, after 12 hours training with 16 CPU and 1 GPU.


Training  
=============
To train a model in atari game, go to `[root]/rlpytorch` and run the following command:

```bash
game=../atari/game model=actor_critic model_file=../atari/model \ 
python3 run.py
    --rom_file [your rom]
    -—batchsize 128         # Batchsize
    —-freq_update 50        # How often the actor used to predict the action gets updated.
    —-num_games 1024        # Num of concurrent games.
    --tqdm                  # If you want to show nice progress bar. 
```

To evaluate, try the following command:
```bash
eval_only=1 game=../atari/game model=actor_critic model_file=../atari/model \ 
python3 run.py
    --num_games 128 --batchsize 32 --tqdm --eval_gpu [your gpu id]
    --rom_file [your rom]
    --load [your model]
    --stats reward          # Accumulate stats.
    --reward_clip -1        # Disable reward_clip
    --num_eval 500          # Number of episodes to be evaluated. 
```

