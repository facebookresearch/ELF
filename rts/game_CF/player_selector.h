#include "ai.h"

class PlayerSelector {
public:
    static AI* GetPlayer(std::string player, int frame_skip) {
        if (player == "flag_simple") return new FlagSimpleAI(INVALID, frame_skip, nullptr);
        else {
            cout << "Unknown player " << player << endl;
            return nullptr;
        }
    }
};
