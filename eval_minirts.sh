MODEL=$1
FRAMESKIP=$2
SEED=245
NUM_EVAL=10000

#game=./rts/game_MC/game model_file=./rts/game_MC/model model=actor_critic python3 eval.py  --num_games 1 --batchsize 1 --tqdm --load ${MODEL} --gpu 4 --players "fs=50,type=AI_NN;fs=${FRAMESKIP},type=AI_SIMPLE" --latest_start 0 --eval_stats winrate --num_eval $NUM_EVAL --additional_labels id,last_terminal,seq --greedy --seed $SEED --save_replay_prefix no_reverse --output_file no_reverse_cout.txt --cmd_dumper_prefix cmd_no_reverse  > no_reverse.txt

#game=./rts/game_MC/game model_file=./rts/game_MC/model model=actor_critic python3 eval.py  --num_games 1 --batchsize 1 --tqdm --load ${MODEL} --gpu 4 --players "fs=50,type=AI_NN;fs=${FRAMESKIP},type=AI_SIMPLE" --latest_start 0 --eval_stats winrate --num_eval $NUM_EVAL --additional_labels id,last_terminal,seq --greedy --seed $SEED --reverse_player --save_replay_prefix reverse --output_file reverse_cout.txt --cmd_dumper_prefix cmd_reverse  > reverse.txt


game=./rts/game_MC/game model_file=./rts/game_MC/model model=actor_critic python3 eval.py  --num_games 1024 --batchsize 128 --tqdm --load ${MODEL} --gpu 4 --players "fs=50,type=AI_NN;fs=${FRAMESKIP},type=AI_SIMPLE" --latest_start 0 --eval_stats winrate --num_eval $NUM_EVAL --additional_labels id,last_terminal,seq --greedy --shuffle_player #--reverse_player 
