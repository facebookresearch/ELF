#pragma once
#include "elf/ai.h"
#include "game_env.h"
#include "game_options.h"

class RTSAction {
    map<UnitId, CmdBPtr> assigned_cmds;
    vector<UICmd> ui_cmds;
};

class RTSState {
public:
    RTSState();

    void Prepare(const RTSGameOptions &options);

    void Save(string *s) const {
        serializer::saver saver(true);
        _env.SaveSnapshot(saver);
        _cmd_receiver.SaveCmdReceiver(saver);
        *s = saver.get_str();
    }

    void Load(const string &s) {
        serializer::loader loader(true);
        loader.set_str(s);
        _env.LoadSnapshot(loader);
        _cmd_receiver.LoadCmdReceiver(loader);
    }

    void LoadSnapshot(const string &filename, bool binary) {
        serializer::loader loader(binary);
        if (! loader.read_from_file(filename)) {
            throw std::range_error("Cannot read from " + filename);
        }
        _env.LoadSnapshot(loader);
        _cmd_receiver.LoadCmdReceiver(loader);
    }

    void SaveSnapshot(const string &filename, bool binary) const {
        serializer::saver saver(binary);
        _env.SaveSnapshot(saver);
        _cmd_receiver.SaveCmdReceiver(saver);
        if (! saver.write_to_file(filename)) {
            throw std::range_error("Cannot write to " + filename);
        }
    }

    int MoveToTick(const std::vector<Tick> &snapshots, float percent) const;

    const GameEnv &env() const { return _env; }
    const CmdReceiver &receiver() const { return _cmd_receiver; }

    void SetUICmdCB(UICallback cb) { _ui_cb = cb; } 
    void SetVerbose(bool verbose) { _verbose = verbose; }
    void SetReplayPrefix(const std::string &prefix) { _save_replay_prefix = prefix; }

    // Function used in GameLoop
    void Init() { }
    void PreAct() { }
    void IncTick() { _cmd_receiver.IncTick(); }
    void Forward(const RTSAction &a);

    elf::GameResult PostAct();
    void Forward(const RTSAction &);
    void Finalize();

    void OnAddPlayer(int player_id);
    void OnRemovePlayer(int player_id);

private:
    GameEnv _env;
    CmdReceiver _cmd_receiver;

    bool _verbose = false;
    std::string _save_replay_prefix;
    Tick _max_tick = 30000;

    elf::GameResult _last_result = elf::GAME_NORMAL;

    std::function<CmdReturn (const UICmd &)> _ui_cb = nullptr;  
};


