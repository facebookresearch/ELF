from queue import Queue, Full, Empty

import msgpack
import msgpack_numpy
msgpack_numpy.patch()

def dumps(obj):
    return msgpack.dumps(obj, use_bin_type=True)

def loads(buf):
    return msgpack.loads(buf)

def check_done_flag(done_flag):
    if done_flag is not None:
        with done_flag.get_lock():
            return done_flag.value
    return False

def queue_get(q, done_flag=None, fail_comment=None):
    if done_flag is None:
        return q.get()
    done = False
    while not done:
        try:
            return q.get(True, 0.01)
        except Empty:
            if fail_comment is not None:
                print(fail_comment)
            if check_done_flag(done_flag):
                done = True
    # Return
    return None

def queue_put(q, item, done_flag=None, fail_comment=None):
    if done_flag is None:
        q.put(item)
        return True
    done = False
    while not done:
        try:
            q.put(item, True, 0.01)
            return True
        except Full:
            if fail_comment is not None:
                print(fail_comment)
            if check_done_flag(done_flag):
                done = True
    return False
