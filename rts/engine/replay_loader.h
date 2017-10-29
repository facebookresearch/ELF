#pragma once

#include "cmd_receiver.h"
#include <atomic>

class RTSState;

class ReplayLoader {
public:
    struct Action {
        vector<CmdBPtr> cmds;
        vector<UICmd> ui_cmds;
        bool restart = false;
        string new_state;
    };

    bool Load(const string& filename);
    void SendReplay(Tick tick, Action *actions);
    void Relocate(Tick tick);

    int GetLoadedReplaySize() const { return _loaded_replay.size(); }
    int GetLoadedReplayLastTick() const { return _loaded_replay.back()->tick(); }

private:
    // Idx for the next replay to send to the queue.
    unsigned int _next_replay_idx = 0;
    vector<CmdBPtr> _loaded_replay;
};

class Replayer : public ReplayLoader {
public:
    using Action = typename ReplayLoader::Action;
    Replayer(const string &filename) { ReplayLoader::Load(filename); }
    virtual bool Act(const RTSState &s, Action *a, const atomic_bool *);
    bool GameEnd() { return true; }

    virtual ~Replayer() { }
};

