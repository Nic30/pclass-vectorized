#include <pcv/utils/benchmark_common.h>
#include <cmath>

namespace pcv {

BenchmarkStats::BenchmarkStats(size_t LOOKUP_CNT, size_t real_rule_cnt,
		size_t trace_cnt, std::ostream &out) :
		LOOKUP_CNT(LOOKUP_CNT), out(out), construction_timer(nullptr), lookup_timer(
				nullptr), real_rule_cnt(real_rule_cnt), number_of_tries_or_tables(
				-1), ns_per_packet(trace_cnt) {
	std::fill(ns_per_packet.begin(), ns_per_packet.end(), 0);
}

void BenchmarkStats::construction_start() {
	construction_timer = new DebugTimer();
}

void BenchmarkStats::construction_stop() {
	construction_timer->stop();
}

void BenchmarkStats::lookup_packet_start() {
	actual_packet_start = std::chrono::high_resolution_clock::now();
}

void BenchmarkStats::lookup_packet_stop(size_t p_id) {
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration < uint64_t, std::nano > t = finish
			- actual_packet_start;
	ns_per_packet[p_id] += t.count();
}

void BenchmarkStats::lookup_start() {
	lookup_timer = new DebugTimer();
}
void BenchmarkStats::lookup_stop() {
	lookup_timer->stop();
}

void BenchmarkStats::set_number_of_tries_or_tables(
		int number_of_tries_or_tables) {
	this->number_of_tries_or_tables = number_of_tries_or_tables;
}

void BenchmarkStats::dump(std::function<void(std::ostream&)> json_extra,
		std::function<void(std::ostream&)> text_extra) {
	uint64_t lookup_time_from_packets = 0;
	for (auto t : ns_per_packet)
		lookup_time_from_packets += t;
	double lookup_speed;
	if (lookup_time_from_packets == 0) {
		// the packet time was not used at all
		lookup_speed = (double(LOOKUP_CNT)/ lookup_timer->us());
	} else {
		lookup_speed =
				((LOOKUP_CNT / double(lookup_time_from_packets)) * 1000.0);
	}
	//out << lookup_time_from_packets << " " << lookup_timer->us() * 1000 << std::endl;
	//auto lookup_speed = (LOOKUP_CNT / double(lookup_timer->us()));
	if (std::isinf(lookup_speed))
		lookup_speed = 0.0; // time can be lower than 1us and json can not contain inf
	out << "{ \"lookup_speed\":" << lookup_speed << "," << std::endl;  // Mpkt/s
	auto overhead = (lookup_timer->us() * 1000
			/ double(lookup_time_from_packets));
	if (std::isinf(overhead))
		overhead = 0.0;

	out << "\"minimal_benchmark_overhead\":" << overhead << "," << std::endl; // (1 = zero overhead caused by precise timers)
	out << "\"construction_time\":" << uint64_t(construction_timer->us()) << "," // us
			<< std::endl;
	out << "\"real_rule_cnt\":" << real_rule_cnt << "," << std::endl;
	out << "\"number_of_tries_or_tables\":" << number_of_tries_or_tables << ","
			<< std::endl;
	out << "\"packet_lookup_times\": [" << std::endl;

	for (size_t i = 0; i < ns_per_packet.size(); i++) {
		auto t = ns_per_packet[i];
		out << t; // ns
		if (i != ns_per_packet.size() - 1) {
			out << ", ";
		}
		if ((i + 1) % 1024 == 0)
			out << std::endl;
	}
	out << "]" << std::endl;

	json_extra(out);
	out << "}";
}

BenchmarkStats::~BenchmarkStats() {
	delete construction_timer;
	delete lookup_timer;
}

}
