MODEL=$1
FRAMESKIP=$2
NUM_EVAL=10000

game=./rts/game_MC/game model_file=./rts/game_MC/model model=actor_critic python3 eval.py  --num_games 1024 --batchsize 128 --tqdm --load ${MODEL} --gpu 0 --players "fs=50,type=AI_NN;fs=${FRAMESKIP},type=AI_SIMPLE" --eval_stats winrate --num_eval $NUM_EVAL --additional_labels id,last_terminal,seq --shuffle_player --num_frames_in_state 1 --greedy #--omit_keys Wt,Wt2,Wt3 #
