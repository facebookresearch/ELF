#include "game_env.h"

float micro_move(Tick tick, const Unit& u, const GameEnv &env, const PointF& target, CmdReceiver *receiver);
bool find_nearby_empty_place(const GameEnv &env, const Unit& u, const PointF &curr, PointF *p_nearby);

constexpr float kDistEps = 1e-3;

