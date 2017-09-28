#pragma once

#include "ai.h"
#include "elf/circular_queue.h"

class TrainedAI : public AIWithComm {
public:
    using Data = typename AIWithComm::Data;

    TrainedAI() : _respect_fow(true), _recent_states(1) { }
    TrainedAI(const AIOptions &opt)
        : AIWithComm(opt.name, opt.fs), _respect_fow(opt.fow), _recent_states(opt.num_frames_in_state) {
      for (auto &v : _recent_states.v()) v.clear();
    }

    bool GameEnd(Tick) override;

    void get_last_pred(ReducedPred *pred) const {
        const auto& gs = ai_comm()->info().data.newest();
        pred->SetPiAndV(gs.pi, gs.V);
    }

protected:
    const bool _respect_fow;

    // History to send.
    CircularQueue<std::vector<float>> _recent_states;

    void compute_state(std::vector<float> *state);

    // Feature extraction.
    void extract(Data *data) override;
    bool handle_response(const Data &data, RTSMCAction *a) override;
};

