#pragma once

#include "tree_search.h"
#include "mcts_ai.h"
#include <type_traits>
#include "member_check.h"

namespace elf {

using namespace std;

MEMBER_FUNC_CHECK(reward);

template <typename S, typename A>
class MCTSStateBaseT : public S {
public:
    using State = S;
    using Action = A;
    using MCTSStateBase = MCTSStateBaseT<S, A>;

    MCTSStateBaseT(const S &state) : S(state), value_(0) { }
    MCTSStateBaseT() : value_(0) { }

    MCTSStateBase &operator=(const State &state) {
        *((State *)this) = state;
        return *this;
    }

    const vector<pair<A, float>> &pi() const { return pi_; }
    float value() const { return value_; }

    template <typename S_ = S, typename enable_if<!has_func_reward<S_>::value>::type * = nullptr>
    float reward() const { return value_; }

protected:
    vector<pair<A, float>> pi_;
    float value_;
};

template <typename S, typename A>
class MCTSStateT : public MCTSStateBaseT<S, A> {
public:
    using MCTSState = MCTSStateT<S, A>;

    MCTSState &operator=(const S &state) {
        *((S *)this) = state;
        return *this;
    }

    bool evaluate() {
        this->get_last_pi(&this->pi_);
        this->value_ = this->get_last_value();
        return true;
    }
};

template <typename AI_>
class MCTSStateAI_T : public MCTSStateBaseT<typename AI_::State, typename AI_::Action> {
public:
    using AI = AI_;
    using State = typename AI::State;
    using Action = typename AI::Action;
    using MCTSStateBase = MCTSStateBaseT<State, Action>;
    using MCTSStateAI = MCTSStateAI_T<AI>;

    MCTSStateAI &operator=(const State &state) {
        *((State *)this) = state;
        return *this;
    }

    bool evaluate(AI *ai) {
        ai->SetState(*this);
        if (! ai->Act(0, nullptr, nullptr)) return false;
        ai->get_last_pi(&this->pi_);
        this->value_ = ai->get_last_value();
        return true;
    }
};

template <typename AI_>
class MCTSStateMT_T : public MCTSStateAI_T<AI_> {
public:
    using AI = AI_;
    using State = typename AI::State;
    using Action = typename AI::Action;
    using MCTSStateAI = MCTSStateAI_T<AI_>;
    using MCTSStateMT = MCTSStateMT_T<AI_>;

    MCTSStateMT_T() { }

    void SetThreadAIs(const vector<AI *>& ai) {
        ai_ = ai;
        thread_id_ = 0;
    }

    MCTSStateMT &operator=(const State &state) {
        *((State *)this) = state;
        return *this;
    }

    void set_thread(int i) { thread_id_ = i; }

    bool evaluate() {
        return MCTSStateAI_T<AI_>::evaluate(ai_[thread_id_]);
    }

private:
    vector<AI *> ai_;
    int thread_id_;
};

// 
template <typename BasicAI> 
class MCTSAI_T : public BasicAI {
public:
    using State = typename BasicAI::State;
    using Action = typename BasicAI::Action;

    using MCTSStateMT = MCTSStateMT_T<BasicAI>;
    using MCTSAI = MCTSAI_T<BasicAI>;
    using MCTSAI_Internal = MCTSAI_Internal_T<MCTSStateMT, Action>; 

    MCTSAI_T(const mcts::TSOptions &options) : mcts_ai_(options), ai_comm_(nullptr) {
        mcts_ai_.SetState(mcts_state_);
    }

    void InitAIComm(AIComm *ai_comm) {
        ai_comm_ = ai_comm;

        // Construct a few DirectPredictAIs.
        const auto &options = mcts_ai_.options();
        ai_.clear(); 
        ai_comms_.clear();

        vector<BasicAI *> ai_dup;

        for (int i = 0; i < options.num_threads; ++i) {
            ai_comms_.emplace_back(ai_comm_->Spawn(i));
            ai_.emplace_back(new DirectPredictAI());
            ai_.back()->InitAIComm(ai_comms_.back().get());
            ai_dup.emplace_back(ai_.back().get());
        }
        mcts_state_.SetThreadAIs(ai_dup);
    } 

protected:
    void on_set_state() override {
        // left the state to MCTSStateMT.
        mcts_state_ = this->s();
    }

    bool on_act(Tick t, Action *a, const std::atomic_bool *done) override {
        return mcts_ai_.Act(t, a, done);
    }

private:
    MCTSAI_Internal mcts_ai_;
    AIComm *ai_comm_;
    MCTSStateMT mcts_state_;

    vector<unique_ptr<AIComm>> ai_comms_;
    vector<unique_ptr<BasicAI>> ai_;
};

// S = full state
// A = full action space.
template <typename S, typename A, typename EmbedAI>
class MCTSAI_Embed_T : public AI_T<S, A> {
public:
    using MCTSAI_Embed = MCTSAI_Embed_T<S, A, EmbedAI>; 
    using MCTSAI_low = MCTSAI_T<EmbedAI>;
    using FullAI = AI_T<S, A>;
    using LowState = typename EmbedAI::State; 
    using LowAction = typename EmbedAI::Action;

    MCTSAI_Embed_T(const mcts::TSOptions &options) : mcts_embed_ai_(options) { 
        mcts_embed_ai_.SetState(low_state_);
    }

protected:
    MCTSAI_low mcts_embed_ai_; 
    LowState low_state_;

    void on_set_state() override {
        // Convert the full state to LowState.
        // Note that MCTS engine has already linked to this state so it can run.
        low_state_ = this->s();
    }

    bool on_act(Tick t, A *a, const std::atomic_bool *done) override {
        LowAction low_a;
        if (! mcts_embed_ai_.Act(t, &low_a, done)) return false;
        *a = low_a;
        return true;
    }
};

} // namespace elf
