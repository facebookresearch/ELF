/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#pragma once

class CommStats {
private:
    // std::mutex _mutex;
    std::atomic<int64_t> _total, _last_total, _min, _max, _sum, _count;
    std::uniform_real_distribution<> _dis;

public:
    CommStats() : _total(0), _last_total(0), _min(0), _max(0), _sum(0), _count(0), _dis(0.5, 1.0) { }

    template <typename Generator>
    void Feed(const int64_t &v, Generator &g) {
        // std::unique_lock<std::mutex> lk{_mutex};
        _total ++;

        if (_last_total <= _total - 1000) {
            // Restart the counting.
            _min = v;
            _max = v;
            _sum = v;
            _count = 1;
            _last_total = _total.load();
        } else {
            // Get min and max.
            if (_min > v) _min = v;
            if (_max < v) _max = v;
            _sum += v;
            _count ++;
        }

        float avg_value = ((float)_sum) / _count;
        float min_value = _min;
        float max_value = _max;
        // lk.release();

        // If we have very large min_max distance, then do a speed control.
        if (max_value - min_value > avg_value / 30) {
            float ratio = (v - min_value) / (max_value  - min_value);
            if (ratio > 0.5) {
                if (_dis(g) <= ratio) {
                   std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }

        /*
        float ratio_diff = it->second.freq / even_freq - 1.0;
        if (ratio_diff >= 0) {
            ratio_diff = std::min(ratio_diff, 1.0f);
            const bool deterministic = false;
            if (deterministic) {
                int wait = int(100 * ratio_diff * 10 + 0.5);  // if (dis(_g) < ratio_diff)
                std::this_thread::sleep_for(std::chrono::microseconds(wait));
            } else {
                std::uniform_real_distribution<> dis(0.0, 1.0);
                if (dis(_g) <= ratio_diff) {
                   std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        }
        */
    }
};


