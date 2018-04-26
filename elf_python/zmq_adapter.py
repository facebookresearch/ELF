# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

# -*- coding: utf-8 -*-
import sys
import zmq

from time import sleep
from .utils import loads, dumps, check_done_flag

__all__ = ["InitSender", "InitConnector", "WaitAll", "SendAll"]

class Sender:
    def __init__(self, name, sender_name, receiver_name, identity, timeout=10000, switch=None):
        self.name = name
        self.sender_name = sender_name
        self.receiver_name = receiver_name
        self.identity = identity
        self.ctx = zmq.Context()

        self.sender = self.ctx.socket(zmq.DEALER)
        self.sender.identity = identity.encode('ascii')
        self.sender.set_hwm(10)
        self.sender.SNDTIMEO = timeout
        self.sender.connect(sender_name)

        self.receiver = self.ctx.socket(zmq.DEALER)
        self.receiver.identity = identity.encode('ascii')
        self.receiver.set_hwm(100)
        self.receiver.RCVTIMEO = timeout
        self.receiver.connect(self.receiver_name)

        self.switch = switch

        sleep(0.2)

    def Send(self, msg, copy=False):
        ''' Send message, no need to specify source, e.g., an environment sends the current state of the game '''
        if self.switch is not None and not self.switch.get(): return False
        try:
            self.sender.send(dumps(msg), copy=copy)
            return True
        except zmq.Again as err:
            # print("Send failed for " + self.identity + "..")
            return False
        except zmq.ZMQError as err:
            print("Send(): ZMQError for " + self.identity + "..")
            return False

    def Receive(self, copy=False):
        if self.switch is not None and not self.switch.get(): return False
        try:
            return loads(self.receiver.recv(copy=False).buffer)
        except zmq.Again as err:
            # print("Receive failed for " + self.identity + "..")
            pass
        except zmq.ZMQError as err:
            print("Receive(): ZMQError for " + self.identity + "..")


class Connector:
    # Constructor
    def __init__(self, name, receiver_name, sender_name, timeout=10000, switch=None):
        self.name = name
        self.sender_name = sender_name
        self.receiver_name = receiver_name

        self.ctx = zmq.Context()
        self.receiver = self.ctx.socket(zmq.ROUTER)
        self.receiver.set_hwm(10)
        self.receiver.RCVTIMEO = timeout
        self.receiver.bind(self.receiver_name)

        self.sender = self.ctx.socket(zmq.ROUTER)
        self.sender.set_hwm(100)
        self.sender.SNDTIMEO = timeout
        self.sender.bind(self.sender_name)

        self.done_flag = None

        self.switch = switch

    def Send(self, data, to=None):
        if self.switch is not None and not self.switch.get(): return False
        try:
            self.sender.send_multipart([to, dumps(data)], copy=False)
            return True
        except zmq.Again as err:
            # If we have nothing to receive, and the program is already
            # existing, break the loop
            print("_thread_reply: Cannot send the package, abandon ...")
            return False
        except zmq.ZMQError as err:
            print("_thread_reply: ZMQ Error ...")
            return False

    def Receive(self):
        if self.switch is not None and not self.switch.get(): return None, True
        try:
            sender_name, m = self.receiver.recv_multipart(copy=False)
            return sender_name, m
        except zmq.Again as err:
            # If we have nothing to receive, and the program is already
            # existing, break the loop
            # print("No package, try again ...")
            if check_done_flag(self.done_flag): return None, None
            sleep(0.005)
            return None, True
        except zmq.ZMQError as err:
            print("ZMQ Error, break the loop ...")
            return None, None

def WaitAll(chs):
    replies = {}
    while True:
        for key, ch in chs.items():
            if key not in replies:
                reply = ch.Receive()
                if reply is not None:
                    replies[key] = reply

        if len(replies) == len(chs):
            break

        sleep(0.005)

    return replies

def SendAll(chs, data):
    sent = set()
    while True:
        for key, ch in chs.items():
            if key not in sent:
                if ch.Send(data[key]):
                    sent.add(key)

        if len(sent) == len(chs):
            break

        sleep(0.005)

def _get_conn_vars(name):
    ipc_request_name = "ipc://bc-req-" + name
    ipc_response_name = "ipc://bc-rep-" + name

    return ipc_request_name, ipc_response_name

def InitSender(name, identity, **kwargs):
    ipc_request_name, ipc_response_name = _get_conn_vars(name)
    return Sender(name, ipc_request_name, ipc_response_name, identity, **kwargs)

def InitConnector(name, **kwargs):
    ipc_request_name, ipc_response_name = _get_conn_vars(name)
    connector = Connector(name, ipc_request_name, ipc_response_name, **kwargs)
    return connector
