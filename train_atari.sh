ROM=$1
GPU=$2

game=./atari/game model=actor_critic model_file=./atari/model python3 train.py --rom_file $ROM --batchsize 4 --freq_update 1 --num_games 16 --tqdm --gpu $GPU --trainer_stats rewards --additional_labels id,last_terminal --keys_in_reply V,rv
