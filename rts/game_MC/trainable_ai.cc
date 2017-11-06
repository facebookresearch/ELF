#include "trainable_ai.h"

#include "engine/game_env.h"
#include "engine/unit.h"
#include "engine/cmd_interface.h"
#include "python_options.h"
#include "state_feature.h"

/*
static inline int sampling(const std::vector<float> &v, std::mt19937 *gen) {
    std::vector<float> accu(v.size() + 1);
    std::uniform_real_distribution<> dis(0, 1);
    float rd = dis(*gen);

    accu[0] = 0;
    for (size_t i = 1; i < accu.size(); i++) {
        accu[i] = v[i - 1] + accu[i - 1];
        if (rd < accu[i]) {
            return i - 1;
        }
    }

    return v.size() - 1;
}
*/

bool TrainedAI::GameEnd() {
    AIWithComm::GameEnd();
    for (auto &v : _recent_states.v()) {
        v.clear();
    }
    return true;
}

void TrainedAI::extract(const State &s, Data *data) {
    GameState *game = &data->newest();

    MCExtractor::SaveInfo(s, id(), game);
    game->name = name();

    if (_recent_states.maxlen() == 1) {
        MCExtractor::Extract(s, id(), _respect_fow, &game->s);
        // std::cout << "(1) size_s = " << game->s.size() << std::endl << std::flush;
    } else {
        std::vector<float> &state = _recent_states.GetRoom();
        MCExtractor::Extract(s, id(), _respect_fow, &state);

        const size_t maxlen = _recent_states.maxlen();
        game->s.resize(maxlen * state.size());
        // std::cout << "(" << maxlen << ") size_s = " << game->s.size() << std::endl << std::flush;
        std::fill(game->s.begin(), game->s.end(), 0.0);
        // Then put all states to game->s.
        for (size_t i = 0; i < maxlen; ++i) {
            const auto &curr_s = _recent_states.get_from_push(i);
            if (! curr_s.empty()) {
                assert(curr_s.size() == state.size());
                std::copy(curr_s.begin(), curr_s.end(), &game->s[i * curr_s.size()]);
            }
        }
    }
}

#define ACTION_GLOBAL 0
#define ACTION_UNIT_CMD 1
#define ACTION_REGIONAL 2

bool TrainedAI::handle_response(const State &s, const Data &data, RTSMCAction *a) {
    a->Init(id(), name());

    // if (_receiver == nullptr) return false;
    const auto &env = s.env();

    // Get the current action from the queue.
    const auto &m = env.GetMap();
    const GameState& gs = data.newest();

    switch(gs.action_type) {
        case ACTION_GLOBAL:
            // action
            //
            {
              string comment = "V: " + std::to_string(gs.V) + ", Prob: ";
              for (int i = 0; i < (int) gs.pi.size(); ++i) {
                  if (i > 0) comment += ", ";
                  comment += std::to_string(gs.pi[i]);
              }
              a->AddComment(comment);

              // Also save prev seen.
              a->AddComment("PrevSeenCount: " + std::to_string(env.GetPrevSeenCount(id())));
            }
            a->SetState9(gs.a);
            break;

        case ACTION_UNIT_CMD:
            {
                // Use gs.unit_cmds
                // std::vector<CmdInput> unit_cmds(gs.unit_cmds);
                // Use data
                std::vector<CmdInput> unit_cmds;
                for (int i = 0; i < gs.n_max_cmd; ++i) {
                    unit_cmds.emplace_back(_XY(gs.uloc[i], m), _XY(gs.tloc[i], m), gs.ct[i], gs.bt[i]);
                }
                std::for_each(unit_cmds.begin(), unit_cmds.end(), [&](CmdInput &ci) { ci.ApplyEnv(env); });
                a->SetUnitCmds(unit_cmds);
            }
            break;

            /*
        case ACTION_REGIONAL:
            {
              if (_receiver->GetUseCmdComment()) {
                string s;
                for (size_t i = 0; i < gs.a_region.size(); ++i) {
                  for (size_t j = 0; j < gs.a_region[0].size(); ++j) {
                    int a = -1;
                    for (size_t k = 0; k < gs.a_region[0][0].size(); ++k) {
                      if (gs.a_region[k][i][j] == 1) {
                        a = k;
                        break;
                      }
                    }
                    s += to_string(a) + ",";
                  }
                  s += "; ";
                }
                SendComment(s);
              }
            }
            // Regional actions.
            return _mc_rule_actor.ActWithMap(env,reply.action_regions, &a->state_string(), &a->cmds());
            */
        default:
            throw std::range_error("action_type not valid! " + to_string(gs.action_type));
    }
    return true;
}
