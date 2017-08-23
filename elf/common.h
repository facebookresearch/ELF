#pragma once

struct SeqInfo {
    int seq;
    int game_counter;
    char last_terminal;

    SeqInfo() : seq(0), game_counter(0), last_terminal(0) { }
    void Inc() { seq ++; last_terminal = 0; }
    void NewEpisode() { seq = 0; game_counter ++; last_terminal = 1; }
};


