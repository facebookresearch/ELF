#pragma once

#include "common.h"

custom_enum(UICtrlType, UI_SLIDEBAR, UI_FASTER_SIMULATION, UI_SLOWER_SIMULATION, UI_CYCLEPLAYER, TOGGLE_GAME_PAUSE, UI_ISSUE_INSTRUCTION,
    UI_FINISH_INSTRUCTION, UI_INTERRUPT_INSTRUCTION);

struct UICmd {
    UICtrlType cmd;
    float arg2;
    string arg3;

    UICmd() { }

    static UICmd GetUIFaster() { UICmd cmd; cmd.cmd = UI_FASTER_SIMULATION; return cmd; }
    static UICmd GetUISlower() { UICmd cmd; cmd.cmd = UI_SLOWER_SIMULATION; return cmd; }
    // Percentage, 0-100.0
    static UICmd GetUISlideBar(float percent) { UICmd cmd; cmd.cmd = UI_SLIDEBAR; cmd.arg2 = percent; return cmd; }
    static UICmd GetUICyclePlayer() { UICmd cmd; cmd.cmd = UI_CYCLEPLAYER; return cmd; }
    static UICmd GetToggleGamePause() { UICmd cmd; cmd.cmd = TOGGLE_GAME_PAUSE; return cmd; }
    static UICmd GetIssueInstruction(const string& instruction) { UICmd cmd; cmd.cmd = UI_ISSUE_INSTRUCTION; cmd.arg3 = instruction; return cmd; }
    static UICmd GetFinishInstruction(const string& instruction) { UICmd cmd; cmd.cmd = UI_FINISH_INSTRUCTION; cmd.arg3 = instruction; return cmd; }
    static UICmd GetInterruptInstruction(const string& instruction) { UICmd cmd; cmd.cmd = UI_INTERRUPT_INSTRUCTION; cmd.arg3 = instruction; return cmd; }

    std::string PrintInfo() const {
        //case UI_SLIDEBAR: oo << make_string("percent:", arg2); break;
        return "";
    }
};

