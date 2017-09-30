#pragma once

#include "tree_search.h"
#include "mcts_ai.h"
#include <type_traits>
#include "member_check.h"

namespace elf {

using namespace std;

template <typename StateWithAI>
class MCTSStateMT_T : public StateWithAI {
public:
    using AI = typename StateWithAI::AI;
    using MCTSStateMT = MCTSStateMT_T<StateWithAI>;
    using State = typename StateWithAI::State;
    using Action = typename StateWithAI::Action;

    MCTSStateMT_T() { }

    void SetThreadAIs(const vector<AI *>& ais) {
        ais_ = ais;
        set_thread(0);
    }

    template <typename U>
    MCTSStateMT &operator=(const U &state) {
        *((State *)this) = state;
        return *this;
    }

    void set_thread(int i) { 
        thread_id_ = i; 
        this->SetAI(ais_[thread_id_]);
    }

private:
    vector<AI *> ais_;
    int thread_id_ = -1;
};

// 
template <typename StateWithAI> 
class MCTSAI_T : public AI_T<typename StateWithAI::State, typename StateWithAI::Action> {
public:
    using AI = typename StateWithAI::AI;
    using State = typename StateWithAI::State;
    using Action = typename StateWithAI::Action;

    using MCTSStateMT = MCTSStateMT_T<StateWithAI>;
    using MCTSAI = MCTSAI_T<StateWithAI>;
    using MCTSAI_Internal = MCTSAI_Internal_T<MCTSStateMT, Action>; 

    MCTSAI_T(const mcts::TSOptions &options) : mcts_ai_(options), ai_comm_(nullptr) {
    }

    void InitAIComm(AIComm *ai_comm) {
        ai_comm_ = ai_comm;

        // Construct a few DirectPredictAIs.
        const auto &options = mcts_ai_.options();
        ai_.clear(); 
        ai_comms_.clear();

        vector<AI *> ai_dup;
        for (int i = 0; i < options.num_threads; ++i) {
            ai_comms_.emplace_back(ai_comm_->Spawn(i));
            ai_.emplace_back(new AI());
            ai_.back()->InitAIComm(ai_comms_.back().get());
            ai_dup.emplace_back(ai_.back().get());
        }
        // cout << "#ai = " << ai_dup.size() << endl;
        mcts_state_.SetThreadAIs(ai_dup);

        // cout << "Done with MCTSAI_T::InitAIComm" << endl;
    } 

    bool Act(const State &s, Action *a, const std::atomic_bool *done) override {
        mcts_state_ = s;
        return mcts_ai_.Act(mcts_state_, a, done);
    }

    template <typename U>
    bool ActOnOtherType(const U &s, Action *a, const std::atomic_bool *done) {
        mcts_state_ = s;
        return mcts_ai_.Act(mcts_state_, a, done);
    }

private:
    MCTSAI_Internal mcts_ai_;
    AIComm *ai_comm_;
    MCTSStateMT mcts_state_;

    vector<unique_ptr<AIComm>> ai_comms_;
    vector<unique_ptr<AI>> ai_;
};

// S = full state
// A = full action space.
template <typename S, typename A, typename LowStateWithAI>
class MCTSAI_Embed_T : public AI_T<S, A> {
public:
    using MCTSAI_Embed = MCTSAI_Embed_T<S, A, LowStateWithAI>;

    using MCTSAI_low = MCTSAI_T<LowStateWithAI>;
    using LowState = typename LowStateWithAI::State; 
    using LowAction = typename LowStateWithAI::Action;

    MCTSAI_Embed_T(const mcts::TSOptions &options) : mcts_embed_ai_(options) { 
    }

    void InitAIComm(AIComm *ai_comm) {
        mcts_embed_ai_.InitAIComm(ai_comm);
    }

    bool Act(const S &s, A *a, const std::atomic_bool *done) override {
        LowAction low_a;
        if (! mcts_embed_ai_.ActOnOtherType(s, &low_a, done)) return false;
        *a = low_a;
        return true;
    }

protected:
    MCTSAI_low mcts_embed_ai_; 
};

} // namespace elf
