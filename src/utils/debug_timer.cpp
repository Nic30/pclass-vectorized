#include <pcv/utils/debug_timer.h>

namespace pcv {

DebugTimer::DebugTimer() {
	start = std::chrono::system_clock::now();
	is_running = true;
}

void DebugTimer::stop() {
	end = std::chrono::system_clock::now();
	is_running = false;
}

size_t DebugTimer::us() {
	if (is_running)
		end = std::chrono::system_clock::now();

	auto us =
			std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	return us;
}

}
