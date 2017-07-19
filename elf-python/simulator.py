from .zmq_adapter import InitSender, WaitAll, SendAll
import abc
import torch.multiprocessing as _mp
mp = _mp.get_context('spawn')

class Simulator(mp.Process):
    def __init__(self, connector_names, id, desc):
        '''
        Example:
        desc = {
            "actor" : (
                dict(s="", last_terminal=""),
                dict(a="")
            ),
            "train" : (
                dict(s="", r="", a=""),
                None
            )
        }
        connector_names = {
            "actor" : "name1",
            "train" : "name2"
        }
        '''
        self.connector_names = connector_names
        self.id = id
        self.desc = desc

    def on_init(self):
        pass

    def restart(self):
        pass

    @abc.abstractproperty
    def terminal(self):
        pass

    @abc.abstractmethod
    def get_key(self, key, *args, **kwargs):
        pass

    @abc.abstractmethod
    def set_key(self, key, value):
        pass

    def run(self):
        '''Return reward'''
        self.chs = { }
        for key, cn in self.connector_names.items():
            self.chs[key] = InitSender(cn, self.id)
        self.on_init()
        self.restart()

        seq = 0
        game_counter = 0

        while True:
            send_chs = {}
            reply_chs = {}
            data = { }
            for key, (input, reply) in self.desc.items():
                # Collector and send data.
                data_to_send = { k : self.get_key(k) for k, _ in input.items() }
                data_to_send.update({
                    "_agent_name" : str("game-") + str(self.id),
                    "_game_counter" : game_counter,
                    "_seq" : seq
                })
                # A batch of size 1
                data[key] = [ data_to_send ]
                send_chs[key] = self.chs[key]

                if reply is not None:
                    reply_chs[key] = self.chs[key]

            # Send all.
            SendAll(send_chs, data)

            # Wait for receive if there is any. Note that if there are multiple
            # desc, then we need to wait simultaneously.
            replies = WaitAll(reply_chs)

            # Set all the keys.
            # Note that each reply is also a batch of size 1
            for key, reply in replies[0].items():
                for k, v in reply.items():
                    self.set_key(k, v)

            if self.terminal:
                seq = 0
                game_counter += 1
                self.restart()
            else:
                seq += 1

