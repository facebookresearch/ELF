#include "ai.h"

class PlayerSelector {
public:
    static AI* GetPlayer(std::string player, int frame_skip) {
        if (player == "td_simple") return new TDSimpleAI(INVALID, frame_skip, nullptr);
        else if (player == "td_built_in") return new TDBuiltInAI(INVALID, frame_skip, nullptr);
        else {
            cout << "Unknown player " << player << endl;
            return nullptr;
        }
    }
};
