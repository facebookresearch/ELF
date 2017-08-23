import threading
from queue import Queue
from collections import defaultdict, OrderedDict
from .utils import queue_get, queue_put

class Replier:
    def __init__(self, ch, reply_batchsize=1, done_flag=None):
        self.reply_batchsize = reply_batchsize
        self.ch = ch
        self.done_flag = done_flag

        # We receive the data to reply.
        self.reply_queue = Queue(maxsize=1000)
        self.reply_cache = OrderedDict()
        self.max_reply_cache = 10000
        self.reply_cache_lock = threading.Lock()
        threading.Thread(target=self._thread_reply).start()

    def _thread_reply(self):
        batch_reply = defaultdict(list)
        while True:
            items = queue_get(self.reply_queue, done_flag=self.done_flag, fail_comment="BatchConnector._thread_reply.queue_get failed, retrying...")

            for item in items:
                if "_key" not in item: continue
                with self.reply_cache_lock:
                    if not item["_key"] in self.reply_cache:
                        self.reply_cache[item["_key"]] = item
                        while len(self.reply_cache) > self.max_reply_cache:
                            del self.reply_cache[next(iter(self.reply_cache.keys()))]

                sender = item["_sender"]
                batch_reply[sender].append(item)
                if len(batch_reply[sender]) >= self.reply_batchsize or item.get("_immediate", False):
                    self.ch.Send(batch_reply[sender], to=sender)
                    batch_reply[sender] = []

    def reply(self, batch_t, data):
        ''' Get a batch at time t, reply with data'''
        if data is None: return

        n = batch_t["_batchsize"]
        senders = batch_t["_sender"][:n]
        agent_names = batch_t["_agent_name"][:n]
        keys = batch_t["_key"][:n]
        seqs = batch_t["_seq"][:n]
        game_counters = batch_t["_game_counter"][:n]

        all_data = []
        for i, (identity, agent_name, seq, key, game_counter) in enumerate(zip(senders, agent_names, seqs, keys, game_counters)):
            this_data = dict(_sender=identity, _agent_name=agent_name, _seq=seq, _key=key, _game_counter=game_counter)
            for k, v in data.items():
                this_data.update({ k : v[i]})
            # print("Reply[%d]: %s" % (i, str(this_data)))
            all_data.append(this_data)
        queue_put(self.reply_queue, all_data, fail_comment="BatchConnector.reply.queue_put failed, retrying...")
