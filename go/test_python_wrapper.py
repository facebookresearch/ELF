import numpy as np
import random
from time import sleep
from datetime import datetime
import game_utils

num_games = 64
batchsize = 16
frame_skip = 50
latest_start = 0

ffi_p = game_utils.initialize_ffi()
context_options, options, infos = \
    game_utils.initialize_game(ffi_p, batchsize, num_games, list_filename = "/data/users/yuandong/go/go_gogod/train.lst")
ffi = ffi_p["ffi"]
C = ffi_p["C"]

context = C.game_initialize(context_options, options)

nIter = 1000
elapsed_wait_only = 0

before = datetime.now()
for k in range(nIter):
   b = datetime.now()
   n = C.game_wait(context, infos, 0)
   elapsed_wait_only += (datetime.now() - b).total_seconds() * 1000

   # Then we move forward.
   C.game_steps(context, n, infos)

elapsed = (datetime.now() - before).total_seconds() * 1000
print("elapsed = %.4lf ms, elapsed_wait_only = %.4lf" % (elapsed, elapsed_wait_only))

# Compute the statistics.
per_loop = elapsed / nIter
per_wait = elapsed_wait_only / nIter
per_frame_loop_n_cpu = per_loop / batchsize / frame_skip
per_frame_wait_n_cpu = per_wait / batchsize / frame_skip

fps_loop = 1000 / per_frame_loop_n_cpu
fps_wait = 1000 / per_frame_wait_n_cpu

print("Time[Loop]: %.6lf ms / loop, %.6lf ms / frame_loop_n_cpu, %.2f FPS" % (per_loop, per_frame_loop_n_cpu, fps_loop))
print("Time[Wait]: %.6lf ms / wait, %.6lf ms / frame_wait_n_cpu, %.2f FPS" % (per_wait, per_frame_wait_n_cpu, fps_wait))
C.game_destroy(context)
