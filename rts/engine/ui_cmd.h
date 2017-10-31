#pragma once

#include "common.h"

custom_enum(UICtrlType, UI_SLIDEBAR, UI_FASTER_SIMULATION, UI_SLOWER_SIMULATION, UI_CYCLEPLAYER, TOGGLE_GAME_PAUSE);

struct UICmd {
    UICtrlType cmd;
    float arg2;

    UICmd() { }

    static UICmd GetUIFaster() { UICmd cmd; cmd.cmd = UI_FASTER_SIMULATION; return cmd; }
    static UICmd GetUISlower() { UICmd cmd; cmd.cmd = UI_SLOWER_SIMULATION; return cmd; }
    // Percentage, 0-100.0
    static UICmd GetUISlideBar(float percent) { UICmd cmd; cmd.cmd = UI_SLIDEBAR; cmd.arg2 = percent; return cmd; }
    static UICmd GetUICyclePlayer() { UICmd cmd; cmd.cmd = UI_CYCLEPLAYER; return cmd; }
    static UICmd GetToggleGamePause() { UICmd cmd; cmd.cmd = TOGGLE_GAME_PAUSE; return cmd; }

    std::string PrintInfo() const {
        //case UI_SLIDEBAR: oo << make_string("percent:", arg2); break;
        return "";
    }
};

