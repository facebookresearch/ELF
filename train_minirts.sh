#!/bin/bash

game=./rts/game_MC/game model=actor_critic model_file=./rts/game_MC/model python3 train.py --batchsize 32 --freq_update 1 --players "type=AI_NN,fs=50,args=backup/AI_SIMPLE|start/500|decay/0.99;type=AI_SIMPLE,fs=20" --num_games 1024 --tqdm --T 20 --additional_labels id,last_terminal --trainer_stats winrate --keys_in_reply V "$@"
