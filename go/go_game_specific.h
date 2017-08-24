#pragma once

#include "elf/pybind_helper.h"
#include "elf/comm_template.h"
#include "elf/hist.h"
#include "elf/copier.hh"

#define BOARD_DIM 19
#define MAX_NUM_FEATURE 25
#define NUM_FUTURE_ACTIONS 3
#define NUM_PLANES 10

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
    using State = GameState;
    // Board state 19x19
    // temp state.
    std::vector<float> features;

    // Next k actions.
    std::vector<long> actions;
    // Seq information.
    int32_t id = -1;
    int32_t seq = 0;
    int32_t game_counter = 0;
    char last_terminal = 0;

    // Reply
    int64_t a;
    float V;
    std::vector<float> pi;
    int32_t rv;

    std::string player_name;

    void Clear() { a = 0; V = 0.0; fill(pi.begin(), pi.end(), 0.0); rv = 0; }

    void Init(int iid, int num_action) {
        id = iid;
        pi.resize(num_action, 0.0);
    }

    GameState &Prepare(const SeqInfo &seq_info) {
        seq = seq_info.seq;
        game_counter = seq_info.game_counter;
        last_terminal = seq_info.last_terminal;

        Clear();
        return *this;
    }

    std::string PrintInfo() const {
        std::stringstream ss;
        ss << "[id:" << id << "][seq:" << seq << "][game_counter:" << game_counter << "][last_terminal:" << last_terminal << "]";
        return ss.str();
    }

    void Restart() {
        seq = 0;
        game_counter = 0;
        last_terminal = 0;
    }
    DECLARE_FIELD(GameState, id, seq, game_counter, last_terminal, features, actions, a, V, pi, rv);
    REGISTER_PYBIND_FIELDS(id, seq, game_counter, last_terminal, features, actions, a, V, pi, rv);
};
