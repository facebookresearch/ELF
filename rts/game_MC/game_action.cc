#include "game_action.h"

bool RTSMCAction::Send(const GameEnv &env, CmdReceiver &receiver) {
    // Apply command. 
    MCRuleActor rule_actor;
    rule_actor.SetPlayerId(_player_id);
    rule_actor.SetReceiver(&receiver);

    vector<int> state(NUM_AISTATE);
    std::fill(state.begin(), state.end(), 0);
    if (_type == CMD_INPUT) {
        rule_actor.ActByCmd(env, _unit_cmds, &_state_string, &_cmds);
    } else {
        bool gather_ok = rule_actor.GatherInfo(env, &_state_string, &_cmds);
        if (! gather_ok) {
            return RTSAction::Send(env, receiver);
        }

        switch(_type) {
            case STATE9: 
                state[_action] = 1;
                break;
            case SIMPLE:
                rule_actor.GetActSimpleState(&state);
                break;
            case HIT_AND_RUN:
                rule_actor.GetActHitAndRunState(&state);
                break;
            default:
                throw std::range_error("Invalid type: " + std::to_string(_type));
        }
        rule_actor.ActByState(env, state, &_state_string, &_cmds);
    }

    return RTSAction::Send(env, receiver);
}

