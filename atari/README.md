## Atari Game

Simulator
=================
The simulator is similar to OpenAI Gym "-v0" setting, in which we apply the same action for k frames, where k is sampled uniformly in {2, 3, 4}.


Compilation
===================
First, install Arcade Learning Environment ([ALE](https://github.com/mgbellemare/Arcade-Learning-Environment)).

Then `mkdir build && cd build && cmake .. && make`

Run `python game.py --rom_file [your game rom file]` to test whether you can run the environment alone.
You can find roms at [atari_py](https://github.com/openai/atari-py/tree/master/atari_py/atari_roms)

To select the Python installation to compile with, use cmake flags `-DPYTHON_EXECUTABLE=/path/to/your/python`
The training code in this repo has to use Python3.

Performance
===============
You should be able to get ~630 mean/864 max, after 12 hours training with 16 CPU and 1 GPU.


Training
=============
To train a model in atari game, go to `[root directory]` and run the following command:

```bash
game=./atari/game model=actor_critic model_file=./atari/model \
python3 run.py
    --rom_file [your rom]
    -—batchsize 128         # Batchsize
    —-freq_update 50        # How often the actor used to predict the action gets updated.
    —-num_games 1024        # Num of concurrent games.
    --tqdm                  # If you want to show nice progress bar.
    --gpu 0                 # Use first gpu
```
`--num_games 256` and `--batchsize 32` should give you 90% GPU usage (on K40M). Here is an example pre-trained model for breakout [link](http://yuandong-tian.com/atari_breakout.bin).

Evaluation
==============
To evaluate, try the following command:

```bash
eval_only=1 game=./atari/game model=actor_critic model_file=./atari/model \
python3 run.py
    --num_games 128 --batchsize 32 --tqdm --eval_gpu [your gpu id]
    --rom_file [your rom]
    --load [your model]
    --stats reward          # Accumulate stats.
    --reward_clip -1        # Disable reward_clip
    --num_eval 500          # Number of episodes to be evaluated.
```

Here is a sample output using the example pre-trained model:

```
$ eval_only=1 game=./atari/game model_file=./atari/model model=actor_critic taskset -c 0-11 python3 run.py --num_games 128 --batchsize 32 --tqdm --load model_breakout.bin --rom_file breakout.bin --reward_clip -1 --num_eval 500

Namespace(T=6, actor_only=False, batchsize=32, discount=0.99, entropy_ratio=0.01, epsilon=0.0, eval=False, eval_freq=10, eval_gpu=1, frame_skip=4, freq_update=1, game_multi=None, gpu=None, grad_clip_norm=None, greedy=False, hist_len=4, load='model_breakout.bin', min_prob=1e-06, num_episode=10000, num_eval=500, num_games=128, num_minibatch=5000, record_dir='./record', reward_clip=-1, rom_dir='./atari', rom_file='breakout.bin', sample_node='pi', sample_policy='epsilon-greedy', save_dir=None, save_prefix='save', stats='rewards', tqdm=True, verbose_collector=False, verbose_comm=False, wait_per_group=False)
A.L.E: Arcade Learning Environment (version 0.5.1)
[Powered by Stella]
Use -help for help screen.
Warning: couldn't load settings file: ./ale.cfg
Game console created:
  ROM file:  ../atari/breakout.bin
  Cart Name: Breakout - Breakaway IV (1978) (Atari)
  Cart MD5:  f34f08e5eb96e500e851a80be3277a56
  Display Format:  AUTO-DETECT ==> NTSC
  ROM Size:        2048
  Bankswitch Type: AUTO-DETECT ==> 2K

Running ROM file...
Random seed is 55070671
Action set: 0 1 3 4
Version:  319c862befeac120903907474c6be205877f1ff1_
Num Actions:  4
Load from model_breakout.bin
Action set: 0 1 3 4
Version:  319c862befeac120903907474c6be205877f1ff1_
Num Actions:  4
100%|███████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████████| 500/500 [15:45<00:00,  1.93s/it]
str_reward: [0] Reward: 643.85/500
```
