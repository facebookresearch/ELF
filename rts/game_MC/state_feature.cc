/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "state_feature.h"

static inline void accu_value(int idx, float val, std::map<int, std::pair<int, float> > &idx2record) {
    auto it = idx2record.find(idx);
    if (it == idx2record.end()) {
        idx2record.insert(make_pair(idx, make_pair(1, val)));
    } else {
        it->second.second += val;
        it->second.first ++;
    }
}

MCExtractorInfo MCExtractor::info_;   // leak?
MCExtractorUsageOptions MCExtractor::usage_;
bool MCExtractor::attach_complete_info_ = false;

void MCExtractor::SaveInfo(const RTSState &s, PlayerId player_id, GameState *gs) {
    gs->tick = s.GetTick();
    gs->winner = s.env().GetWinnerId();
    gs->terminal = s.env().GetTermination() ? 1 : 0;

    gs->last_r = 0.0;

    // 测试获取基地位置
    // UnitId baseId = s.env().FindClosestBase(player_id);
    //const Unit* base = s.env().GetUnit(s.env().FindClosestBase(player_id));
    //gs->base_x = s.env().GetUnit(s.env().FindClosestBase(player_id))->GetPointF().x;
    //gs->base_y = s.env().GetUnit(s.env().FindClosestBase(player_id))->GetPointF().y;
    // 测试获取基地位置

    int winner = s.env().GetWinnerId();
    if (winner != INVALID) {
      if (winner == player_id) gs->last_r = 1.0;
      else gs->last_r = -1.0;
      // cout << "player_id: " << player_id << " " << (gs->last_r > 0 ? "Won" : "Lose") << endl;
    }
}

void MCExtractor::Extract(const RTSState &s, PlayerId player_id, bool respect_fow, std::vector<float> *state) {
    // [Channel, width, height]
    const int kTotalChannel = attach_complete_info_ ? 2 * info_.size() : info_.size();
    const GameEnv &env = s.env();
    const auto &m = env.GetMap();
    const int sz = kTotalChannel * m.GetXSize() * m.GetYSize();
    state->resize(sz);
    std::fill(state->begin(), state->end(), 0.0);

    extract(s, player_id, respect_fow, &(*state)[0]);
    if (attach_complete_info_) {
        extract(s, player_id, false, &(*state)[m.GetXSize() * m.GetYSize() * info_.size()]);
    }
}


void MCExtractor::extract(const RTSState &s, PlayerId player_id, bool respect_fow, float *state) {
    assert(player_id != INVALID);
   
    const GameEnv &env = s.env();
   
    const Extractor *ext_ut = info_.get("UnitType");
    const Extractor *ext_feature = info_.get("Feature");
    const Extractor *ext_resource = info_.get("Resource");

    // The following features are optional and might be nullptr.
    const Extractor *ext_hist_bin = info_.get("HistBin"); // leak?
    const Extractor *ext_ut_prev_seen = info_.get("UnitTypePrevSeen");
    const Extractor *ext_hist_bin_prev_seen = info_.get("HistBinPrevSeen");

    const int kAffiliation = ext_feature->Get(MCExtractorInfo::AFFILIATION);
    const int kChHpRatio = ext_feature->Get(MCExtractorInfo::HP_RATIO);
    const int kChBuildSinceDecay = ext_feature->Get(MCExtractorInfo::HISTORY_DECAY);

    const auto &m = env.GetMap();

    std::map<int, std::pair<int, float> > idx2record;

    PlayerId visibility_check = respect_fow ? player_id : INVALID;

    Tick tick = s.GetTick();

    GameEnvAspect aspect(env, visibility_check);
    UnitIterator unit_iter(aspect, UnitIterator::ALL);

    const Player &player = env.GetPlayer(player_id);

    float total_hp_ratio = 0.0;

    int myworker = 0;
    int mytroop = 0;
    int mybarrack = 0;
    float base_hp_level = 0.0;

    while (! unit_iter.end()) {
        const Unit &u = *unit_iter;
        int x = int(u.GetPointF().x);
        int y = int(u.GetPointF().y);
        float hp_level = u.GetProperty()._hp / (u.GetProperty()._max_hp + 1e-6);
        float build_since = 50.0 / (tick - u.GetBuiltSince() + 1);
        UnitType t = u.GetUnitType();

        bool self_unit = (u.GetPlayerId() == player_id);

        accu_value(_OFFSET(ext_ut->Get(t), x, y, m), 1.0, idx2record);

        // Self unit or enemy unit.
        // For historical reason, the flag of enemy unit = 2
        accu_value(_OFFSET(kAffiliation, x, y, m), (self_unit ? 1 : 2), idx2record);
        accu_value(_OFFSET(kChHpRatio, x, y, m), hp_level, idx2record);

        if (usage_.type >= BUILD_HISTORY) {
            accu_value(_OFFSET(kChBuildSinceDecay, x, y, m), build_since, idx2record);
            if (ext_hist_bin != nullptr) {
                int h_idx = ext_hist_bin->Get(tick - u.GetBuiltSince());
                accu_value(_OFFSET(h_idx, x, y, m), 1, idx2record);
            }
        }

        total_hp_ratio += hp_level;

        if (self_unit) {
            if (t == WORKER) myworker += 1;
            else if (t == MELEE_ATTACKER || t == RANGE_ATTACKER) mytroop += 1;
            else if (t == BARRACKS) mybarrack += 1;
            else if (t == BASE) base_hp_level = hp_level;
       }

        ++ unit_iter;
    }

    // Add seen objects.
    if (ext_ut_prev_seen != nullptr && ext_hist_bin_prev_seen != nullptr && usage_.type >= PREV_SEEN) {
        for (int x = 0; x < m.GetXSize(); ++x) {
            for (int y = 0; y < m.GetYSize(); ++y) {
                Loc loc = m.GetLoc(x, y);
                const Fog &f = player.GetFog(loc);
                if (usage_.type == ONLY_PREV_SEEN && f.CanSeeTerrain()) continue;
                for (const auto &u : f.seen_units()) {
                    UnitType t = u.GetUnitType();
                    int t_idx = ext_ut_prev_seen->Get(t);
                    accu_value(_OFFSET(t_idx, x, y, m), 1.0, idx2record);

                    int h_idx = ext_hist_bin_prev_seen->Get(tick - u.GetBuiltSince());
                    accu_value(_OFFSET(h_idx, x, y, m), 1, idx2record);
                }
            }
        }
    }

    for (const auto &p : idx2record) {
        state[p.first] = p.second.second / p.second.first;
    }

    myworker = min(myworker, 3);
    mytroop = min(mytroop, 5);
    mybarrack = min(mybarrack, 1);

    // Add resource layer for the current player.
    const int c_idx = ext_resource->Get(player.GetResource());
    const int c = _OFFSET(c_idx, 0, 0, m);
    std::fill(state + c, state + c + m.GetXSize() * m.GetYSize(), 1.0);

    if (usage_.type >= BUILD_HISTORY) {
        const int kChBaseHpRatio = ext_feature->Get(MCExtractorInfo::BASE_HP_RATIO);
        const int c = _OFFSET(kChBaseHpRatio, 0, 0, m);
        std::fill(state + c, state + c + m.GetXSize() * m.GetYSize(), base_hp_level);
    }
}
