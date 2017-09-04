ROM=$1
MODEL=$2
GPU=$3

game=./atari/game model=actor_critic model_file=./atari/model python3 eval.py --num_games 128 --batchsize 32 --tqdm --gpu $GPU --rom_file $ROM --load $MODEL --eval_stats rewards --reward_clip -1 --num_eval 500 --additional_labels id,last_terminal 
