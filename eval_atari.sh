PYTHON=python3
MODEL=../private_models/atari_breakout.bin

eval_only=1 game=./atari/game model=actor_critic model_file=./atari/model $PYTHON run.py --num_games 128 --batchsize 32 --tqdm --eval_gpu 4 --rom_file breakout.bin --load $MODEL --stats rewards --reward_clip -1 --num_eval 500 
