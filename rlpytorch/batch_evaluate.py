# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

from subprocess import check_output
import json
import argparse
import re
import os

parser = argparse.ArgumentParser()
parser.add_argument("--input", type=str)
parser.add_argument("--output", type=str)
parser.add_argument("--first_n", type=int, default=16)
parser.add_argument("--game_path", type=str, default="../atari")
parser.add_argument("--log_prefix", type=str, default="log_")
parser.add_argument("--keys", type=str, help="Key and defaut values to transfer to evaluation process, e.g., rom_file=,reward_clip=-1", default="")
parser.add_argument("--filters", type=str, help="Filters, e.g., batchsize<=32,rom_file!=breakout.bin", default="")

args = parser.parse_args()

env_vars = "LD_PRELOAD=/home/yuandong/libc/libc-2.17.so LD_PRELOAD=/home/yuandong/gcc-4.9.3/lib64/libstdc++.so.6.0.20 LD_LIBRARY_PATH=/home/yuandong/protobuf-3.0.0/src/.libs:/home/yuandong/zeromq-4.2.0/src/.libs:/home/yuandong/anaconda3/lib:/home/yuandong/Arcade-Learning-Environment"

command = '''
$(ENV) eval_only=1 game=$(GAME_PATH)/game model_file=$(GAME_PATH)/model model=actor_critic taskset -c $(CPU_RANGE) python3 run.py --num_games 128 --batchsize 32 --load $(LOAD) --tqdm $(MORE_OPTIONS) 2>&1 1> $(LOG).log &
'''

records = json.load(open(args.input))
output = open(args.output, "w")

i = 0
step = 6

default_record = { }
for key_entry in args.keys.split(","):
    items = key_entry.split("=")
    key = items[0]
    if len(items) == 1:
        default_value = None
    else:
        default_value = items[1]
    default_record[key] = default_value

# make filter a function
funcs = []
splitter = re.compile("^(.*?)(>|<|>=|<=|=|!=)(.*?)$")
for cond in args.filters.split(","):
    m = splitter.match(cond)
    if m:
        funcs.append(eval("lambda record : record[\"%s\"] %s %s" % (m.group(1), m.group(2), m.group(3))))

log_dir = os.path.dirname(args.log_prefix)
if len(log_dir) > 0:
    output.write("mkdir " + log_dir + "\n")

for record in records:
    if i >= args.first_n: break
    if any([not func(record) for func in funcs]): continue

    idx = i // 2
    used_record = { k : v for k, v in record.items() if k in default_record }
    used_record.update({ k : v for k, v in default_record.items() if v is not None})
    used_record["eval_gpu"] = str(idx)
    # Put used_record there.
    more_options = " ".join(["--%s %s" % (k, v) for k, v in used_record.items()])

    special_tokens = dict(
        ENV = env_vars,
        CPU_RANGE = "%d-%d" % (idx * step, idx * step + step - 1),
        LOG = "%s%d" % (args.log_prefix, i),
        MORE_OPTIONS = more_options,
        LOAD = record["last_model"],
        GAME_PATH = args.game_path
    )
    this_command = str(command)
    for k, v in special_tokens.items():
        this_command = this_command.replace("$(%s)" % k, str(v))

    output.write("\n")
    output.write("# idx = %d/%d\n" % (i, len(records)))
    output.write("\n".join(("# %s=%s" % (k, v) for k, v in record.items())))
    output.write("\n")
    output.write(this_command)
    output.write("\n")

    i += 1

output.close()

