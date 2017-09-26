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
#include "ws_server.h"
#include "ai.h"
#include "raw2cmd.h"

class TCPAI : public AI {

private:
    RawToCmd _raw_converter;
    int _vis_after;

    std::unique_ptr<WSServer> server_;
    moodycamel::ConcurrentQueue<std::string> queue_;

    string save_vis_data() const;
    bool send_vis(const string &s);

protected:
    void on_set_id() override {
        this->AI::on_set_id();
        _raw_converter.SetId(id());
    }

    bool on_act(Tick t, RTSMCAction *action, const std::atomic_bool *) override;

public:
    // If player_id == INVALID, then it will send the full information.
    TCPAI(const std::string &name, int vis_after, int port)
        : AI(name, 1), _vis_after(vis_after) {
          server_.reset(
              new WSServer{port, [this](const std::string& msg) {
                this->queue_.enqueue(msg);
              }});
        }

    // This is for visualization.
    /*
    bool IsUnitSelected(UnitId id) const override { return _raw_converter.IsUnitSelected(id); }
    vector<int> GetAllSelectedUnits() const override {
      auto selected = _raw_converter.GetAllSelectedUnits();
      return vector<int>(selected.begin(), selected.end());
    }
    */
};
