#pragma once
#include <chrono>

namespace pcv {

/**
 * The timer used to measure duration for debugging purposes
 * */
class DebugTimer {
	std::chrono::system_clock::time_point start;
	std::chrono::system_clock::time_point end;
	bool is_running;
public:
	DebugTimer();
	void stop();

	/*
	 * Get microseconds
	 * */
	size_t us();
};

}
