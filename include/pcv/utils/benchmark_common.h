#pragma once
#include <pcv/utils/debug_timer.h>
#include <iostream>
#include <functional>

namespace pcv {

class BenchmarkStats {
public:
	size_t LOOKUP_CNT;
	std::ostream & out;
	DebugTimer * construction_timer;
	DebugTimer * lookup_timer;
	size_t real_rule_cnt;
	int number_of_tries_or_tables;

	std::chrono::high_resolution_clock::time_point actual_packet_start;
	std::vector<uint64_t> ns_per_packet;

	BenchmarkStats(size_t LOOKUP_CNT, size_t real_rule_cnt, size_t trace_cnt,
			std::ostream & out = std::cout);
	void construction_start();
	void construction_stop();

	// for a single packet
	void lookup_packet_start();
	void lookup_packet_stop(size_t p_id);

	// the lookup as a whole
	void lookup_start();
	void lookup_stop();

	void set_number_of_tries_or_tables(int number_of_tries_or_tables);

	void dump(std::function<void(std::ostream &)> json_extra =
			[](std::ostream &) {},
			std::function<void(std::ostream &)> text_extra =
					[](std::ostream &) {});
	~BenchmarkStats();
};

}
