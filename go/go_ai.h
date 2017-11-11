#pragma once

#include "ai.h"
#include <fstream>

using namespace std;

class DirectPredictAI: public AIWithComm {
public:
    using Data = AIWithComm::Data;

    void SetActorName(const std::string &name) {
        actor_name_ = name;
    }

    void get_last_pi(vector<pair<Coord, float>> *output_pi, ostream *oo = nullptr) const {
        assert(last_state_ != nullptr);

        if (data().size() == 0) {
            cout << "DirectPredictAI: history empty!" << endl;
            throw std::range_error("DirectPredictAI: history empty!");
        }

        if (oo != nullptr) {
            *oo <<  last_state_->ShowBoard() << endl << endl;
        }

        const vector<float> &pi = data().newest().pi;

        output_pi->clear();
        const auto &bf = last_state_->last_extractor();
        for (size_t i = 0; i < pi.size(); ++i) {
            Coord m = bf.Action2Coord(i);
            output_pi->push_back(make_pair(m, pi[i]));
        }
        // sorting..
        using data_type = pair<Coord, float>;
        std::sort(output_pi->begin(), output_pi->end(), [](const data_type &d1, const data_type &d2) {
            return d1.second > d2.second;
        });
        // Then we only pick the first 5 (check invalid move first).
        vector<data_type> tmp;
        int i = 0;
        while (tmp.size() < 5) {
            if (i >= (int)output_pi->size()) break;
            const data_type& v = output_pi->at(i);
            // Check whether this move is right.
            bool valid = last_state_->CheckMove(v.first);
            if (valid) {
                tmp.push_back(v);
            }

            if (oo != nullptr) {
                *oo << "Predict [" << i << "][" << coord2str(v.first) << "][" << coord2str2(v.first) << "][" << v.first << "] " << v.second;
                if (valid) *oo << " added" << endl;
                else *oo << " invalid" << endl;
            }
            i ++;
        }
        *output_pi = tmp;
    }

    float get_last_value() const {
        assert(last_state_ != nullptr);
        return data().newest().V;
    }

protected:
    std::string actor_name_;
    const GoState *last_state_ = nullptr;

    void extract(const GoState &state, Data *data) override {
        auto &gs = data->newest();
        gs.name = actor_name_;
        gs.move_idx = state.GetPly();
        gs.winner = 0;
        const auto &bf = state.last_extractor();
        bf.Extract(&gs.s);
        last_state_ = &state;
    }

    bool handle_response(const GoState &s, const Data &data, Coord *c) override {
        auto action = data.newest().a;
        if (c != nullptr) *c = s.last_extractor().Action2Coord(action);
        return true;
    }
};

// An AI surrogate for human playing.
class HumanPlayer : public AIWithComm {
public:
    void SetActorName(const std::string &name) {
        actor_name_ = name;
    }

protected:
    std::string actor_name_;

    void extract(const GoState &state, Data *data) override {
        auto &gs = data->newest();
        gs.name = actor_name_;
        gs.move_idx = state.GetPly();
        gs.winner = 0;
        const auto &bf = state.last_extractor();
        bf.Extract(&gs.s);
    }

    bool handle_response(const GoState &s, const Data &data, Coord *c) override {
        auto action = data.newest().a;
        if (c != nullptr) *c = s.last_extractor().Action2Coord(action);
        return true;
    }
};
