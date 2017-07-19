from .zmq_adapter import InitSender, WaitAll, SendAll
import abc
import torch.multiprocessing as _mp
mp = _mp.get_context('spawn')

class Simulator(mp.Process):
    '''
    Wrapper for simulator.
    Functions to override:
        on_init: Initialization after the process has started.
        restart: restart the environment.
        terminal: property that tell whether the game has reached terminal
        get_key: from the key, get the content. e.g. ``get_key("s")`` will give the encoded state of the game.
        set_key: set the key from replies. e.g., ``set_key("a", 2)`` set the action to be 2 (and the underlying game can continue).
    '''
    def __init__(self, id, desc):
        '''
        Example:
        desc = dict(
            actor = dict(
                input = dict(s="", last_terminal=""),
                reply = dict(a="")
                connector = "name1"
            ),
            train = dict(
                input = dict(s="", r="", a=""),
                reply = None,
                connector = "name2"
            )
        }
        '''
        super(Simulator, self).__init__()
        self.id = id
        self.agent_name = "game-" + self.id
        self.desc = desc

    def on_init(self):
        pass

    def restart(self):
        pass

    @abc.abstractproperty
    def terminal(self):
        pass

    @abc.abstractmethod
    def get_key(self, key):
        pass

    @abc.abstractmethod
    def set_key(self, key, value):
        pass

    def run(self):
        '''Return reward'''
        self.chs = { }
        for key, v in self.desc.items():
            self.chs[key] = InitSender(v["connector"], self.id)

        self.seq = 0
        self.game_counter = 0
        self.restart()

        while True:
            send_chs = {}
            reply_chs = {}
            reply_format = {}
            data = { }
            for name, v in self.desc.items():
                # Collector and send data.
                data_to_send = { k : self.get_key(k) for k, _ in v["input"].items() }
                data_to_send.update({
                    "_agent_name" : self.agent_name,
                    "_game_counter" : self.game_counter,
                    "_seq" : self.seq
                })
                # A batch of size 1
                data[name] = [ data_to_send ]
                send_chs[name] = self.chs[name]

                if v["reply"] is not None:
                    reply_chs[name] = self.chs[name]
                    reply_format[name] = v["reply"]

            # Send all.
            # print("[%s]: Before send ..." % agent_name)
            SendAll(send_chs, data)

            # Wait for receive if there is any. Note that if there are multiple
            # desc, then we need to wait simultaneously.
            # print("[%s]: Before wait reply ..." % agent_name)
            replies = WaitAll(reply_chs)

            # Set all the keys.
            for name, reply in replies.items():
                # Note that each reply is also a batch of size 1
                for k, v in reply[0].items():
                    if k in reply_format[name]:
                        self.set_key(k, v)

            if self.terminal:
                self.seq = 0
                self.game_counter += 1
                self.restart()
            else:
                self.seq += 1

