#pragma once

#include "../elf/pybind_helper.h"
#include "../elf/comm_template.h"
#include "../elf/fields_common.h"


#define BOARD_DIM 19
#define MAX_NUM_FEATURE 25
#define NUM_FUTURE_ACTIONS 3
#define NUM_PLANES 10

// Reply function, action/value.
struct Reply {
    int action;
    float value;
    std::vector<float> prob;
    Reply(int action = 0, float value = 0.0) : action(action), value(value) { }
    void Clear() { action = 0; value = 0.0; fill(prob.begin(), prob.end(), 0.0);}
};

struct GameOptions {
    // Seed.
    unsigned int seed;
    int num_planes;
    int num_future_actions;
    // A list file containing the files to load.
    std::string list_filename;
    REGISTER_PYBIND_FIELDS(seed, num_planes, num_future_actions, list_filename);
};

struct GameState {
    // Board state 19x19
    // temp state.
    float features[MAX_NUM_FEATURE][BOARD_DIM][BOARD_DIM];

    // Next k actions.
    long actions[NUM_FUTURE_ACTIONS];
};

using GameInfo = InfoT<GameState, Reply>;
using Context = ContextT<GameOptions, GameState, Reply>;

using DataAddr = typename Context::DataAddr;
using AIComm = typename Context::AIComm;
using Comm = typename Context::Comm;


class FieldState : public FieldT<AIComm, float> {
public:
    void ToPtr(int batch_idx, const AIComm& ai_comm) override {
        const auto &info = ai_comm.newest(this->_hist_loc);
        const float *start = (const float *) info.data.features;
        std::copy(start, start + MAX_NUM_FEATURE * BOARD_DIM * BOARD_DIM, this->addr(batch_idx));
    }
};

class FieldActions : public FieldT<AIComm, long> {
public:
    void ToPtr(int batch_idx, const AIComm& ai_comm) override {
        const auto &info = ai_comm.newest(this->_hist_loc);
        const long *start = (const long *) info.data.actions;
        std::copy(start, start + NUM_FUTURE_ACTIONS, this->addr(batch_idx));
    }
};

// DEFINE_LAST_REWARD(AIComm, float, data.last_reward);
// DEFINE_REWARD(AIComm, float, data.last_reward);
DEFINE_POLICY_DISTR(AIComm, float, reply.prob);
//
// DEFINE_TERMINAL(AIComm, unsigned char);
// DEFINE_LAST_TERMINAL(AIComm, unsigned char);

FIELD_SIMPLE(AIComm, Value, float, reply.value);
FIELD_SIMPLE(AIComm, Action, int64_t, reply.action);

using DataAddr = DataAddrT<AIComm>;
using DataAddrService = DataAddrServiceT<AIComm>;

bool CustomFieldFunc(int batchsize, const std::string& key, const std::string& v, SizeType *sz, FieldBase<AIComm> **p);
