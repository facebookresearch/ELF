Utils
=====
To facilitate design, we also provide tools.

ArgsProvider
------------
An end-to-end training procedure usually contains multiple components, each with a lot of arguments, or hyperparameters (e.g., game hyperparameters, simulator hyperparameters and optimization hyperparameters). Putting them all together in the one main file so that they can be loaded from a common command line, is not only messy but also not extensible. In ELF, each Python class can register its own arguments in one argument group, as well as borrow others' arguments. This is realized using :class:`ArgsProvider`.

Using this functionality, the arguments are grouped by classes, as shown in the following:
::
    $ python3 run.py --help
	usage: run.py [-h] [--sample_policy {epsilon-greedy,multinomial,uniform}]
				  [--sample_node SAMPLE_NODE] [--greedy] [--epsilon EPSILON]
				  [--freq_update FREQ_UPDATE] [--record_dir RECORD_DIR]
				  [--save_prefix SAVE_PREFIX] [--save_dir SAVE_DIR]
				  [--handicap_level HANDICAP_LEVEL] [--latest_start LATEST_START]
				  [--latest_start_decay LATEST_START_DECAY] [--fs_ai FS_AI]
				  [--fs_opponent FS_OPPONENT]
				  [--ai_type {AI_SIMPLE,AI_HIT_AND_RUN,AI_NN,AI_FLAG_NN,AI_TD_NN}]
				  [--opponent_type {AI_SIMPLE,AI_HIT_AND_RUN,AI_FLAG_SIMPLE,AI_TD_BUILT_IN}]
				  [--max_tick MAX_TICK] [--mcts_threads MCTS_THREADS]
				  [--seed SEED] [--simple_ratio SIMPLE_RATIO]
				  [--ratio_change RATIO_CHANGE] [--actor_only]
				  [--num_games NUM_GAMES] [--batchsize BATCHSIZE]
				  [--game_multi GAME_MULTI] [--T T] [--eval] [--wait_per_group]
				  [--verbose_comm] [--verbose_collector]
				  [--num_minibatch NUM_MINIBATCH] [--num_episode NUM_EPISODE]
				  [--tqdm] [--gpu GPU] [--load LOAD]
				  [--entropy_ratio ENTROPY_RATIO]
				  [--grad_clip_norm GRAD_CLIP_NORM] [--discount DISCOUNT]
				  [--min_prob MIN_PROB]

	optional arguments:
	  -h, --help            show this help message and exit

	Sampler:
	  --sample_policy {epsilon-greedy,multinomial,uniform}
							Sample policy
	  --sample_node SAMPLE_NODE
	  --greedy
	  --epsilon EPSILON     Used in epsilon-greedy approach

	Trainer:
	  --freq_update FREQ_UPDATE
	  --record_dir RECORD_DIR
	  --save_prefix SAVE_PREFIX
	  --save_dir SAVE_DIR

	Loader:
	  --handicap_level HANDICAP_LEVEL
	  --latest_start LATEST_START
	  --latest_start_decay LATEST_START_DECAY
	  --fs_ai FS_AI
	  --fs_opponent FS_OPPONENT
	  --ai_type {AI_SIMPLE,AI_HIT_AND_RUN,AI_NN,AI_FLAG_NN,AI_TD_NN}
	  --opponent_type {AI_SIMPLE,AI_HIT_AND_RUN,AI_FLAG_SIMPLE,AI_TD_BUILT_IN}
	  --max_tick MAX_TICK   Maximal tick
	  --mcts_threads MCTS_THREADS
	  --seed SEED
	  --simple_ratio SIMPLE_RATIO
	  --ratio_change RATIO_CHANGE
	  --actor_only

	ContextArgs:
	  --num_games NUM_GAMES
	  --batchsize BATCHSIZE
	  --game_multi GAME_MULTI
	  --T T
	  --eval
	  --wait_per_group
	  --verbose_comm
	  --verbose_collector

	SingleProcessRun:
	  --num_minibatch NUM_MINIBATCH
	  --num_episode NUM_EPISODE
	  --tqdm

	ModelLoader:
	  --gpu GPU             gpu to use
	  --load LOAD           load model

	ActorCritic:
	  --entropy_ratio ENTROPY_RATIO
							The entropy ratio we put on A3C
	  --grad_clip_norm GRAD_CLIP_NORM
							Gradient norm clipping
	  --discount DISCOUNT
	  --min_prob MIN_PROB   Minimal probability used in training

.. currentmodule:: rlpytorch

.. autoclass:: ArgsProvider
    :members:

    .. automethod:: __init__

* :doc:`rlpytorch`. Go back.
