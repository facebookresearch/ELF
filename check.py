import argparse
from datetime import datetime

import sys
import os

from rlpytorch import *

if __name__ == '__main__':
    parser = argparse.ArgumentParser()

    collector = StatsCollector()
    game = load_module(os.environ["game"]).Loader()
    runner = SingleProcessRun()

    args_providers = [game, runner]

    all_args = ArgsProvider.Load(parser, args_providers)

    GC = game.initialize()
    # GC.setup_gpu(0)

    GC.reg_callback("actor", collector.actor)
    GC.reg_callback("train", collector.train)

    runner.setup(GC)
    runner.run()
