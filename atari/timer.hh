#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <iostream>
#include <string>

class Timer {
	public:
		using Clock = std::chrono::high_resolution_clock;
		Timer() {
			restart();
		}

		// return current unix timestamp
		void restart() {
			m_start_time = std::chrono::high_resolution_clock::now();
		}

		// return duration in seconds
		double duration() const {
			auto now = std::chrono::high_resolution_clock::now();
			auto m = std::chrono::duration_cast<std::chrono::microseconds>(now - m_start_time).count();
			return m * 1.0 / 1e6;
		}

	protected:
		std::chrono::time_point<Clock> m_start_time;

};


class GuardedTimer: public Timer {
	public:
		GuardedTimer(const std::string& msg,  bool enabled=true):
			GuardedTimer([msg](double duration){
					std::cout << msg << ": " << std::to_string(duration * 1000.) << " milliseconds." << std::endl;
				})
		{ enabled_ = enabled; }

		GuardedTimer(const char* msg, bool enabled=true):
			GuardedTimer(std::string(msg), enabled) {}

		GuardedTimer(std::function<void(double)> callback):
			m_callback(callback)
		{ }

		~GuardedTimer() {
			if (enabled_)
				m_callback(duration());
		}

	protected:
		bool enabled_;
		std::function<void(double)> m_callback;

};

