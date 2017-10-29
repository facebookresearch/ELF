import sys

from elf import GCWrapper, ContextArgs, MoreLabels
from rlpytorch import ArgsProvider
import abc

class CommonLoader:
    def __init__(self, module):
        self.context_args = ContextArgs()
        self.more_labels = MoreLabels()
        self.module = module

        basic_define_args = [
            ("handicap_level", 0),
            ("players", dict(type=str, help=";-separated player infos. For example: type=AI_NN,fs=50,args=backup/AI_SIMPLE|decay/0.99|start/1000,fow=True;type=AI_SIMPLE,fs=50")),
            ("max_tick", dict(type=int, default=30000, help="Maximal tick")),
            ("shuffle_player", dict(action="store_true")),
            ("num_frames_in_state", 1),
            ("max_unit_cmd", 1),
            ("seed", 0),
            ("actor_only", dict(action="store_true")),
            ("model_no_spatial", dict(action="store_true")), # TODO, put it to model
            ("save_replay_prefix", dict(type=str, default=None)),
            ("output_file", dict(type=str, default=None)),
            ("cmd_dumper_prefix", dict(type=str, default=None)),
            ("gpu", dict(type=int, help="gpu to use", default=None)),
        ]

        self.args = ArgsProvider(
            call_from = self,
            define_args = basic_define_args + self._define_args(),
            more_args = ["num_games", "batchsize", "T"],
            child_providers = [ self.context_args.args, self.more_labels.args ]
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

        player_infos = []
        for i, player in enumerate(players_str.split(";")):
            ai_options = self.module.AIOptions()
            ai_options.num_frames_in_state = self.args.num_frames_in_state
            info = dict()
            for item in player.split(","):
                key, value = item.split("=")
                self._set_key(ai_options, key, value)
                info[key] = value

            if player_names is not None:
                self._set_key(ai_options, "name", player_names[i])
                info["name"] = player_names[i]

            opt.AddAIOptions(ai_options)
            player_infos.append(info)

        return player_infos

    def _init_gc(self, player_names=None):
        args = self.args

        co = self.module.ContextOptions()
        self.context_args.initialize(co)

        opt = self.module.PythonOptions()
        opt.seed = args.seed
        opt.shuffle_player = args.shuffle_player
        opt.max_unit_cmd = args.max_unit_cmd
        opt.max_tick = args.max_tick
        # [TODO] Put it to TD.
        opt.handicap_level = args.handicap_level

        player_infos = self._parse_players(opt, player_names)

        # opt.output_filename = b"simulators.txt"
        # opt.output_filename = b"cout"
        if args.save_replay_prefix is not None:
            opt.save_replay_prefix = args.save_replay_prefix.encode('ascii')
        if args.output_file is not None:
            opt.output_filename = args.output_file.encode("ascii")
        if args.cmd_dumper_prefix is not None:
            opt.cmd_dumper_prefix = args.cmd_dumper_prefix.encode("ascii")

        print("Options:")
        opt.Print()

        print("ContextOptions:")
        co.print()

        GC = self.module.GameContext(co, opt)
        self._on_gc(GC)

        params = GC.GetParams()
        print("Version: ", GC.Version())
        print("Num Actions: ", params["num_action"])
        print("Num unittype: ", params["num_unit_type"])
        print("num planes: ", params["num_planes"])
        params["rts_engine_version"] = GC.Version()
        params["players"] = player_infos

        return co, GC, params

    def _define_args(self):
        return []

    def _on_gc(self, GC):
        pass

    @abc.abstractmethod
    def _get_train_spec(self):
        pass

    @abc.abstractmethod
    def _get_actor_spec(self):
        pass

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

        self.more_labels.add_labels(desc)

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

        desc["actor0"]["name"] = reference_name
        desc["actor1"]["name"] = train_name

        if not args.actor_only:
            # For training, we want input, action (filled by actor models), value (filled by actor models) and reward.
            desc["train1"] = self._get_train_spec()
            desc["train1"]["name"] = train_name

        self.more_labels.add_labels(desc)

        params.update(dict(
            num_group = 1 if args.actor_only else 2,
            action_batchsize = int(desc["actor0"]["batchsize"]),
            train_batchsize = int(desc["train1"]["batchsize"]) if not args.actor_only else None,
            T = args.T,
            model_no_spatial = args.model_no_spatial
        ))

        return GCWrapper(GC, co, desc, gpu=args.gpu, use_numpy=False, params=params)

    def initialize_reduced_service(self):
        args = self.args

        reference_name = "reference"
        train_name = "train"
        co, GC, params = self._init_gc(player_names=[train_name, reference_name])

        desc = {}
        # For actor model, no reward needed, we only want to get input and return distribution of actions.
        # sampled action and and value will be filled from the reply.
        desc["reduced_project"] = self._get_reduced_project()
        desc["reduced_forward"] = self._get_reduced_forward()
        desc["reduced_predict"] = self._get_reduced_predict()
        if params["players"][1]["type"] == "AI_NN":
            desc["actor"] = self._get_actor_spec()
            desc["actor"]["batchsize"] //= 2
            desc["actor"]["name"] = "reference"

        self.more_labels.add_labels(desc)

        return GCWrapper(GC, co, desc, gpu=args.gpu, use_numpy=False, params=params)

