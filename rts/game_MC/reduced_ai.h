#pragma once

#include "ai.h"
#include "state_feature.h"

using namespace std;

namespace reduced {

class ProjectAI : public elf::AIWithCommT<RTSState, vector<float>, AIComm> {
public:
    using AIWithComm = elf::AIWithCommT<RTSState, vector<float>, AIComm>;
    using Data = typename AIWithComm::Data;
    using State = typename AIWithComm::State;

    ProjectAI() { }
    ProjectAI(const AIOptions &opt) : AIWithComm(opt.name) { }

protected:
    // Feature extraction.
    void extract(const State &s, Data *data) override {
        auto &gs = data->newest();
        MCExtract(s, id(), true, &gs.s);
        MCSaveInfo(s, id(), &gs);
        gs.name = "reduced_project";
    }

    bool handle_response(const State &, const Data &data, vector<float> *a) override {
        *a = data.newest().reduced_s;
        return true;
    }
};

class ForwardAI : public elf::AIWithCommT<ReducedState, vector<float>, AIComm> {
public:
    using AIWithComm = elf::AIWithCommT<ReducedState, vector<float>, AIComm>;
    using Data = typename AIWithComm::Data;
    using State = typename AIWithComm::State;

    ForwardAI() { }
    ForwardAI(const AIOptions &opt) : AIWithComm(opt.name) { }

protected:
    // Feature extraction.
    void extract(const State &s, Data *data) override {
        auto &gs = data->newest();
        gs.reduced_s = s.state;
        gs.a = s.action;
        gs.name = "reduced_forward";
    }

    bool handle_response(const State &, const Data &data, vector<float> *a) override {
        *a = data.newest().reduced_next_s;
        return true;
    }
};

class PredictAI : public elf::AIWithCommT<vector<float>, ReducedPred, AIComm> {
public:
    using AIWithComm = elf::AIWithCommT<vector<float>, ReducedPred, AIComm>;
    using Data = typename AIWithComm::Data;
    using State = typename AIWithComm::State;

    PredictAI() { }
    PredictAI(const AIOptions &opt) : AIWithComm(opt.name) { }

protected:
    // Feature extraction.
    void extract(const State &s, Data *data) override {
        auto &gs = data->newest();
        gs.reduced_s = s;
        gs.name = "reduced_predict";
    }

    bool handle_response(const State &, const Data &data, ReducedPred *a) override {
        a->SetPiAndV(data.newest().pi, data.newest().V);
        return true;
    }
};

}  // namespace reduced
