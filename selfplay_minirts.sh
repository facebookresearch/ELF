MODEL=$1

game=./rts/game_MC/game model_file=./rts/game_MC/model model=actor_critic python3 selfplay.py  --num_games 1024 --batchsize 128 --tqdm --latest_start 0 --players "fs=50,type=AI_NN;fs=50,type=AI_NN" --trainer_stats winrate --additional_labels id,last_terminal,seq --load ${MODEL} --gpu 2 --T 20 --shuffle_player 
