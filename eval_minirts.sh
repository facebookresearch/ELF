MODEL=$1
FRAMESKIP=$2

eval_only=1 game=./rts/game_MC/game model_file=./rts/game_MC/model model=actor_critic taskset -c 12-23 python3 eval.py  --num_games 128 --batchsize 32 --tqdm --load ${MODEL} --gpu 4 --players "fs=50,type=AI_NN;fs=${FRAMESKIP},type=AI_SIMPLE" --latest_start 0 --eval_stats winrate --num_eval 10000 --additional_labels id,last_terminal
