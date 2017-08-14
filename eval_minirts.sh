MODEL=$1
FRAMESKIP=$2

eval_only=1 game=./rts/game_MC/game model_file=./rts/game_MC/model model=actor_critic taskset -c 12-23 python3 run.py  --num_games 128 --batchsize 32 --tqdm --load ${MODEL} --eval_gpu 4 --fs_opponent ${FRAMESKIP} --latest_start 0 --opponent_type AI_SIMPLE --stats winrate --num_eval 10000 --additional_labels id,last_terminal
