#pragma once
#include <pcv/utils/debug_timer.h>
#include <iostream>
#include <functional>

namespace pcv {

class BenchmarkStats {
public:
	size_t LOOKUP_CNT;
	bool dump_as_json;
	std::ostream & out;
	DebugTimer * construction_timer;
	DebugTimer * lookup_timer;
	size_t real_rule_cnt;
	int number_or_tries_or_tables;

	BenchmarkStats(size_t LOOKUP_CNT, bool dump_as_json, size_t real_rule_cnt, std::ostream & out =
			std::cout);
	void construction_start();
	void construction_stop();

	void lookup_start();
	void lookup_stop();

	void set_number_or_tries_or_tables(int number_or_tries_or_tables);

	void dump(std::function<void(std::ostream &)> json_extra =
			[](std::ostream &) {},
			std::function<void(std::ostream &)> text_extra =
					[](std::ostream &) {});
	~BenchmarkStats();
};

}
