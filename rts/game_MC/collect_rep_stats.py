# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
import shlex
import glob
import re
import json
import tqdm
import multiprocessing
from subprocess import call

matcher = re.compile(r"\[(\d):(.*?)\] V: ([-\.\d]+), Prob: (.*?)$")
win_matcher = re.compile("\[(\d+)\] player (\d+):(\d+) won")
prev_seen_matcher = re.compile("\[(\d):(.*?)\] PrevSeenCount: (\d+)")

def get_comment(line):
    items = shlex.split(line)
    if items[0] != "CmdComment":
        return None
    return items[5]

def get_content(f):
    values = []
    policies = []
    prev_seen_counts = []

    last_comment = None
    for line in open(f):
        if line.strip() == "": break
        comment = get_comment(line)
        if comment is None: continue

        last_comment = comment

        m = matcher.match(comment)
        if m:
            player_id = int(m.group(1))
            value = float(m.group(3))
            policy = [ float(v) for v in m.group(4).split(",") ]
            values.append(value)
            policies.append(policy)
            continue

        m = prev_seen_matcher.match(comment)
        if m:
            prev_seen_counts.append(int(m.group(3)))
            continue

    if last_comment is not None:
        m = win_matcher.match(last_comment)
        if m:
            time_spent = int(m.group(1))
            result = "win" if m.group(2) == "0" else "lose"

            return dict(
                result=result,
                time_spent=time_spent,
                value=values,
                policy=policies,
                prev_seen_counts=prev_seen_counts,
                filename=f
            )

filenames = list(glob.glob("*.rep"))

all_records = dict(win=[], lose=[])

p = multiprocessing.Pool(64)
num_valid = 0
for f, res in zip(tqdm.tqdm(filenames, ncols=100), p.imap(get_content, filenames)):
    if res is None: continue
    all_records[res["result"]].append(res)
    num_valid += 1

prefix = sys.argv[1]
root_path = "/home/yuandong/MiniRTSData"

print("#Valid record: %d" % num_valid)
json.dump(all_records, open(os.path.join(root_path, "summary_" + prefix + ".json"), "w"))

# compress_cmd = "tar cvf %s \"*.rep\"" % os.path.join(root_path, prefix + ".tar")
# call(compress_cmd.split())
