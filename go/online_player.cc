#include "online_player.h"

////////////////////////////////////// Online Player ////////////////////////////////////////////////
void OnlinePlayer::SaveTo(GameState &gs) {
    gs.move_idx = _state.GetPly();
    gs.winner = 0;
    const auto &bf = _state.extractor();
    bf.Extract(&gs.s);
}

void OnlinePlayer::Next(int64_t action) {
    // From action to coord.
    _state.last_board() = _state.board();
    Coord m = _state.last_extractor().Action2Coord(action);
    // Play it.
    if (! _state.ApplyMove(m)) {
        cout << "Invalid action! action = " << action << " x = " << X(m) << " y = " << Y(m) << coord2str(m) << " please try again" << endl;
    }
}


