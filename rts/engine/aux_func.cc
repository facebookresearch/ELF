#include "aux_func.h"
#include "cmd.h"
#include "cmd.gen.h"

/*
static float trunc(float v, float b) {
    return std::max(std::min(v, b), -b);
}
*/
#define MT_OK 0
#define MT_TARGET_INVALID 1
#define MT_ARRIVED 2
#define MT_CANNOT_MOVE 3

static int move_toward(const RTSMap& m, const UnitTemplate& unit_def, float speed, const UnitId& id,
        const PointF& curr, const PointF& target, PointF *move) {
    // Given curr location, move towards the target.
    PointF diff;

    if (! PointF::Diff(target, curr, &diff)) return MT_TARGET_INVALID;
    if (std::abs(diff.x) < kDistEps && std::abs(diff.y) < kDistEps) return MT_ARRIVED;

    //bool movable = false;
    while (true) {
        diff.Trunc(speed);
        PointF next_p(curr);
        next_p += diff;

        bool movable = m.CanPass(next_p, id, unit_def);
        // cout << "MoveToward [" << id << "]: Try straight: " << next_p << " movable: " << movable << endl;

        if (! movable) {
            next_p = curr;
            next_p += diff.CCW90();
            movable = m.CanPass(next_p, id, unit_def);
            // cout << "MoveToward [" << id << "]: Try CCW: " << next_p << " movable: " << movable << endl;
        }
        if (! movable) {
            next_p = curr;
            next_p += diff.CW90();
            movable = m.CanPass(next_p, id, unit_def);
            // cout << "MoveToward [" << id << "]: Try CW: " << next_p << " movable: " << movable << endl;
        }

        // If we still cannot move, then we reduce the speed.
        if (movable) {
            *move = next_p;
            return MT_OK;
        }
        else return MT_CANNOT_MOVE;

        /*
        speed /= 1.2;
        // If the move speed is too slow, then we skip.
        if (speed < 0.005) break;
        */
    }
}

float micro_move(Tick tick, const Unit& u, const GameEnv &env, const PointF& target, CmdReceiver *receiver) {
    const RTSMap &m = env.GetMap();
    const PointF &curr = u.GetPointF();
    const Player &player = env.GetPlayer(u.GetPlayerId());
    const UnitTemplate& unit_def = env.GetGameDef().unit(u.GetUnitType());

    // cout << "Micro_move: Current: " << curr << " Target: " << target << endl;
    float dist_sqr = PointF::L2Sqr(target, curr);
    const UnitProperty &property = u.GetProperty();

    static const int kMaxPlanningIteration = 1000;

    // Move to a target. Ideally we should do path-finding, for now just follow L1 distance.
    if ( property.CD(CD_MOVE).Passed(tick) ) {
        PointF move;

        float dist_sqr = PointF::L2Sqr(curr, target);
        PointF waypoint = target;
        bool planning_success = false;

        if (dist_sqr > 1.0) {
            // Do path planning.
            Coord first_block;
            float est_dist;
            planning_success = player.PathPlanning(tick, u.GetId(), unit_def, curr, target,
                kMaxPlanningIteration, receiver->GetPathPlanningVerbose(), &first_block, &est_dist);
            if (planning_success && first_block.x >= 0 && first_block.y >= 0) {
                waypoint.x = first_block.x;
                waypoint.y = first_block.y;
            }
        }
        // cout << "micro_move: (" << curr << ") -> (" << waypoint << ") planning: " << planning_success << endl;
        int ret = move_toward(m, unit_def, property._speed, u.GetId(), curr, waypoint, &move);
        if (ret == MT_OK) {
            // Set actual move.
            receiver->SendCmd(CmdIPtr(new CmdTacticalMove(u.GetId(), move)));
        } else if (ret == MT_CANNOT_MOVE) {
            // Unable to move. This is usually due to PathPlanning issues.
            // Too many such commands will leads to early termination of game.
            // [TODO]: Make PathPlanning better.
            receiver->GetGameStats().RecordFailedMove(tick, 1.0);
        }
    }
    return dist_sqr;
}

bool find_nearby_empty_place(const GameEnv& env, const Unit& u, const PointF &curr, PointF *p_nearby) {
    const RTSMap &m = env.GetMap();
    const UnitTemplate& unit_def = env.GetGameDef().unit(u.GetUnitType());

    PointF nn;
    nn = curr.Left(); if (m.CanPass(nn, INVALID, unit_def)) { *p_nearby = nn; return true; }
    nn = curr.Right(); if (m.CanPass(nn, INVALID, unit_def)) { *p_nearby = nn; return true; }
    nn = curr.Up(); if (m.CanPass(nn, INVALID, unit_def)) { *p_nearby = nn; return true; }
    nn = curr.Down(); if (m.CanPass(nn, INVALID, unit_def)) { *p_nearby = nn; return true; }

    nn = curr.LT(); if (m.CanPass(nn, INVALID, unit_def)) { *p_nearby = nn; return true; }
    nn = curr.LB(); if (m.CanPass(nn, INVALID, unit_def)) { *p_nearby = nn; return true; }
    nn = curr.RT(); if (m.CanPass(nn, INVALID, unit_def)) { *p_nearby = nn; return true; }
    nn = curr.RB(); if (m.CanPass(nn, INVALID, unit_def)) { *p_nearby = nn; return true; }

    nn = curr.LL(); if (m.CanPass(nn, INVALID, unit_def)) { *p_nearby = nn; return true; }
    nn = curr.RR(); if (m.CanPass(nn, INVALID, unit_def)) { *p_nearby = nn; return true; }
    nn = curr.TT(); if (m.CanPass(nn, INVALID, unit_def)) { *p_nearby = nn; return true; }
    nn = curr.BB(); if (m.CanPass(nn, INVALID, unit_def)) { *p_nearby = nn; return true; }

    return false;
}

