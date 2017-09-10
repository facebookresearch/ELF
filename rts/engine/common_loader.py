import sys

from elf import GCWrapper, ContextArgs
from rlpytorch import ArgsProvider

class CommonLoader:
    def __init__(self, module):
        self.context_args = ContextArgs()
        self.module = module

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
                ("handicap_level", 0),
                ("players", dict(type=str, help=";-separated player infos. For example: type=AI_NN,fs=50,args=backup/AI_SIMPLE|decay/0.99|start/1000,fow=True;type=AI_SIMPLE,fs=50")),
                ("max_tick", dict(type=int, default=30000, help="Maximal tick")),
                ("shuffle_player", dict(action="store_true")),
                ("mcts_threads", 64),
                ("num_frames_in_state", 1),
                ("seed", 0),
                ("actor_only", dict(action="store_true")),
                ("additional_labels", dict(type=str, default=None, help="Add additional labels in the batch. E.g., id,seq,last_terminal")),
                ("model_no_spatial", dict(action="store_true")), # TODO, put it to model
                ("save_replay_prefix", dict(type=str, default=None)),
                ("output_file", dict(type=str, default=None)),
                ("cmd_dumper_prefix", dict(type=str, default=None)),
                ("gpu", dict(type=int, help="gpu to use", default=None)),
            ],
            more_args = ["batchsize", "T"],
            child_providers = [ self.context_args.args ]
        )

    def _set_key(self, ai_options, key, value):
        if not hasattr(ai_options, key):
            print("AIOptions does not have key = " + key)
            return

        # Can we automate this?
        bool_convert = dict(t=True, true=True, f=False, false=False)

        try:
            if key == "fow":
                setattr(ai_options, key, bool_convert[value.lower()])
            elif key == "name" or key == "args" or key == "type":
                setattr(ai_options, key, value)
            else:
                setattr(ai_options, key, int(value))
        except ValueError:
            print("Value error! key = " + str(key) + " value = " + str(value))
            sys.exit(1)

    def _parse_players(self, opt, player_names):
        players_str = str(self.args.players)
        if players_str[0] == "\"" and players_str[-1] == "\"":
            players_str = players_str[1:-1]

        for i, player in enumerate(players_str.split(";")):
            ai_options = self.module.AIOptions()
            ai_options.num_frames_in_state = self.args.num_frames_in_state
            for item in player.split(","):
                key, value = item.split("=")
                self._set_key(ai_options, key, value)
            if player_names is not None:
                self._set_key(ai_options, "name", player_names[i])
            opt.AddAIOptions(ai_options)

    def _init_gc(self, player_names=None):
        args = self.args

        co = self.module.ContextOptions()
        self.context_args.initialize(co)

        opt = self.module.PythonOptions()
        opt.seed = args.seed
        opt.shuffle_player = args.shuffle_player
        opt.mcts_threads = args.mcts_threads
        opt.mcts_rollout_per_thread = 50
        opt.max_tick = args.max_tick
        # [TODO] Put it to TD.
        opt.handicap_level = args.handicap_level

        self._parse_players(opt, player_names)

        # opt.output_filename = b"simulators.txt"
        # opt.output_filename = b"cout"
        if args.save_replay_prefix is not None:
            opt.save_replay_prefix = args.save_replay_prefix.encode('ascii')
        if args.output_file is not None:
            opt.output_filename = args.output_file.encode("ascii")
        if args.cmd_dumper_prefix is not None:
            opt.cmd_dumper_prefix = args.cmd_dumper_prefix.encode("ascii")
        opt.Print()

        GC = self.module.GameContext(co, opt)
        params = GC.GetParams()
        print("Version: ", GC.Version())
        print("Num Actions: ", params["num_action"])
        print("Num unittype: ", params["num_unit_type"])
        params["rts_engine_version"] = GC.Version()

        return co, GC, params

    def _add_more_labels(self, desc):
        args = self.args
        if args.additional_labels is None: return

        extra = args.additional_labels.split(",")
        for _, v in desc.items():
            v["input"]["keys"].update(extra)

    def _add_player_name(self, desc, player_name):
        desc["filters"] = dict(player_name=player_name)

    def initialize(self):
        co, GC, params = self._init_gc()
        args = self.args

        desc = {}
        # For actor model, no reward needed, we only want to get input and return distribution of actions.
        # sampled action and and value will be filled from the reply.
        desc["actor"] = self._get_actor_spec()

        if not args.actor_only:
            # For training, we want input, action (filled by actor models), value (filled by actor models) and reward.
            desc["train"] = self._get_train_spec()

        self._add_more_labels(desc)

        params.update(dict(
            num_group = 1 if args.actor_only else 2,
            action_batchsize = int(desc["actor"]["batchsize"]),
            train_batchsize = int(desc["train"]["batchsize"]) if not args.actor_only else None,
            T = args.T,
            model_no_spatial = args.model_no_spatial
        ))

        return GCWrapper(GC, co, desc, gpu=args.gpu, use_numpy=False, params=params)

    def initialize_selfplay(self):
        args = self.args
        reference_name = "reference"
        train_name = "train"

        co, GC, params = self._init_gc(player_names=[train_name, reference_name])

        desc = {}
        # For actor model, no reward needed, we only want to get input and return distribution of actions.
        # sampled action and and value will be filled from the reply.
        desc["actor0"] = self._get_actor_spec()
        desc["actor1"] = self._get_actor_spec()

        self._add_player_name(desc["actor0"], reference_name)
        self._add_player_name(desc["actor1"], train_name)

        if not args.actor_only:
            # For training, we want input, action (filled by actor models), value (filled by actor models) and reward.
            desc["train1"] = self._get_train_spec()
            self._add_player_name(desc["train1"], train_name)

        self._add_more_labels(desc)

        params.update(dict(
            num_group = 1 if args.actor_only else 2,
            action_batchsize = int(desc["actor0"]["batchsize"]),
            train_batchsize = int(desc["train1"]["batchsize"]) if not args.actor_only else None,
            T = args.T,
            model_no_spatial = args.model_no_spatial
        ))

        return GCWrapper(GC, co, desc, gpu=args.gpu, use_numpy=False, params=params)
