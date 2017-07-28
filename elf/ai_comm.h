#pragma once
#include "pybind_helper.h"
#include "python_options_utils_cpp.h"
#include "common.h"

#include <random>

// Game information
template <typename _Data>
struct InfoT {
    using Data = _Data;
    using State = typename Data::State;

    // Meta info for this game.
    const MetaInfo meta;

    // Game data (state, etc)
    // Note that data should have the following methods
    // Prepare(SeqInfo), initialize everything.
    Data data;

    InfoT(int id) : meta(id) { }
    InfoT(const InfoT<Data> &parent, int child_id) : meta(parent.meta, child_id) { }
};

// Communication between main_loop and AI (which is in a separate thread).
// main_loop will send the environment data to AI, and call AI's Act().
// In Act(), AI will compute the best move and return it back.
template <typename _Comm>
class AICommT {
public:
    using Comm = _Comm;
    using AIComm = AICommT<Comm>;
    using Data = typename Comm::Data;
    using Info = typename Comm::Info;
    using DataPrepareReturn = decltype(std::declval<Data>().Prepare(SeqInfo()));

private:
    Comm *_comm;

    Info _info;
    SeqInfo _curr_seq;

    std::mt19937 _g;

public:
    AICommT(int id, Comm *comm) : _comm(comm), _info(id), _g(_info.meta.query_id) {
        // TODO Make _data constructor more generalizable.
        _info.data.InitHist(comm->GetT());
    }

    AICommT(const AIComm& parent, int child_id)
        : _comm(parent._comm), _info(parent._info, child_id), _curr_seq(parent._curr_seq), _g(_info.meta.query_id) {
    }

    DataPrepareReturn Prepare() {
        // we move the history forward.
        DataPrepareReturn ret = _info.data.Prepare(_curr_seq);
        _curr_seq.Inc();
        return ret;
    }

    const Info &info() const { return _info; }
    Info &info() { return _info; }

    std::mt19937 &gen() { return _g; }

    const SeqInfo &seq_info() const { return _curr_seq; }

    // Once we set all the information, send the state.
    bool SendDataWaitReply() {
        // Compute a hash code for data.
        // curr()->hash_code = serializer::hash_obj(curr()->data);
        /*
        if (curr().data.reward != 0.0) {
            std::cout << "[k=" << _meta.id << "][seq=" << curr().seq - 1 << "] "
                      << "Last_reward: " << curr().data.reward << std::endl;
        }
        */
        // std::cout << "[" << _meta.id << "] Before SendDataWaitReply" << std::endl;
        return _comm->SendDataWaitReply(_info.meta.query_id, _info);
        // std::cout << "[" << _meta.id << "] Done with SendDataWaitReply, continue" << std::endl;
    }

    void Restart() {
        _curr_seq.NewEpisode();
    }

    // Spawn Child. The pointer will be own by the caller.
    AIComm *Spawn(int child_id) const {
        AIComm *child = new AIComm(*this, child_id);
        return child;
    }

    REGISTER_PYBIND;
};
