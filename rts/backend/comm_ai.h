/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#pragma once
#include <zmq.h>
#include <czmq.h>
#include "../../vendor/CZMQ-ZWSSock/zwssock.h"

#include "../engine/omni_ai.h"
#include "raw2cmd.h"

class TCPAI : public OmniAI {
private:
    RawToCmd _raw_converter;
    int _vis_after;

    zctx_t* _ctx;
    zwssock_t* _sock;
    zframe_t* _id;

    string save_vis_data(const GameEnv& env) const;
    bool send_vis(const string &s);

protected:
    void on_set_id(PlayerId id) override { 
        this->OmniAI::on_set_id(id);
        _raw_converter.SetId(id); 
    }

    bool send_cmd_anyway() const override { 
        if (_player_id == INVALID) return true; 
        else return this->OmniAI::send_cmd_anyway();
    }

public:
    TCPAI() { }
    
    // If player_id == INVALID, then it will send the full information.
    TCPAI(PlayerId id, int vis_after, char *address, CmdReceiver *receiver)
        : OmniAI(id, 1, receiver), _raw_converter(id), _vis_after(vis_after) {

        _ctx = zctx_new();
        _sock = zwssock_new_router(_ctx);
        zwssock_bind(_sock, address);
        zmsg_t* msg;
        msg = zwssock_recv(_sock);
        _id = zmsg_pop(msg);
        zmsg_destroy(&msg);
    }

    bool Act(const GameEnv &env, bool must_act = false) override;

    // This is for visualization.
    bool IsUnitSelected(UnitId id) const override { return _raw_converter.IsUnitSelected(id); }
    vector<int> GetAllSelectedUnits() const override {
        auto selected = _raw_converter.GetAllSelectedUnits();
        return vector<int>(selected.begin(), selected.end());
    }

    ~TCPAI() {
      zwssock_destroy(&_sock);
	  zctx_destroy(&_ctx);
      zframe_destroy(&_id);
    }
};
