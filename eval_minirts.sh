MODEL=$1
FRAMESKIP=$2
NUM_EVAL=500

game=./rts/game_MC/game model_file=./rts/game_MC/model_unit_cmd model=actor_critic python3 eval.py  --num_games 1 --batchsize 1 --tqdm --load ${MODEL} --gpu 0 --players "fs=5,type=AI_NN;fs=${FRAMESKIP},type=AI_SIMPLE" --eval_stats winrate --num_eval $NUM_EVAL --additional_labels id,last_terminal,seq  --num_frames_in_state 1 --greedy --keys_in_reply V --save_replay_prefix reply --use_unit_action #--omit_keys Wt,Wt2,Wt3 #--shuffle_player
