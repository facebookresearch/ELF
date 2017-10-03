#pragma once

#include "ai.h"
#include "elf/circular_queue.h"

class TrainedAI : public AIWithComm {
public:
    using State = AIWithComm::State;
    using Data = typename AIWithComm::Data;

    TrainedAI() : _respect_fow(true), _recent_states(1) { }
    TrainedAI(const AIOptions &opt)
        : AIWithComm(opt.name), _respect_fow(opt.fow), _recent_states(opt.num_frames_in_state) {
      for (auto &v : _recent_states.v()) v.clear();
    }

    bool GameEnd(const State &) override;

protected:
    const bool _respect_fow;

    // History to send.
    CircularQueue<std::vector<float>> _recent_states;

    void compute_state(std::vector<float> *state);

    // Feature extraction.
    void extract(const State &, Data *data) override;
    bool handle_response(const State &, const Data &data, RTSMCAction *a) override;
};

