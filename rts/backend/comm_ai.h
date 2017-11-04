/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once
#include <concurrentqueue.h>
#include "vendor/ws_server.h"
#include "ai.h"
#include "raw2cmd.h"
#include "engine/replay_loader.h"
#include "save2json.h"

class WebCtrl {
public:
    WebCtrl(int port) {
        server_.reset(
            new WSServer{port, [this](const std::string& msg) {
                this->queue_.enqueue(msg);
        }});
    }

    PlayerId GetId() const {
        return _raw_converter.GetPlayerId();
    }

    void SetId(PlayerId id) {
        _raw_converter.SetId(id);
    }

    void Receive(const RTSState &s, vector<CmdBPtr> *cmds, vector<UICmd> *ui_cmds);
    void Send(const string &s) { server_->send(s); }
    void Extract(const RTSState &s, json *game);

private:
    RawToCmd _raw_converter;
    std::unique_ptr<WSServer> server_;
    moodycamel::ConcurrentQueue<std::string> queue_;
};


class TCPAI : public AI {
public:
    TCPAI(const std::string &name, int port) : AI(name), _ctrl(port) { }
    bool Act(const State &s, RTSMCAction *action, const std::atomic_bool *) override;

private:
    WebCtrl _ctrl;

protected:
    void on_set_id() override {
        this->AI::on_set_id();
        _ctrl.SetId(id());
    }
};

class TCPSpectator : public Replayer {
public:
    using Action = typename Replayer::Action;

    TCPSpectator(const string &replay_filename, int vis_after, int port)
      : Replayer(replay_filename), _ctrl(port), _vis_after(vis_after) {
       _ctrl.SetId(INVALID);
    }

    bool Act(const RTSState &s, Action *action, const std::atomic_bool *) override;

private:
    WebCtrl _ctrl;
    int _vis_after;

    vector<string> _history_states;
};
