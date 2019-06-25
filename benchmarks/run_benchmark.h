#pragma once
#include <vector>
#include <array>
#include <pcv/common/range.h>
#include <pcv/utils/benchmark_common.h>

template<class CLS_T>
void run_benchmark(CLS_T & cls,
		const std::vector<std::array<pcv::Range1d<uint16_t>, 2>> & rules,
		const std::vector<std::array<uint16_t, 2>> & packets,
		size_t LOOKUP_CNT) {
	pcv::BenchmarkStats stats(LOOKUP_CNT, true, rules.size());
	{
		stats.construction_start();
		size_t i = 0;
		for (auto & r : rules) {
			cls.insert( { r, i });
			i++;
		}
		stats.construction_stop();
	}

	{
		stats.lookup_start();
		// [TODO] assert enough large rule_cnt so cache is flushed
		for (size_t i = 0; i < LOOKUP_CNT; i++) {
			auto & p = packets[i % packets.size()];
			cls.search(p);
		}
		stats.lookup_stop();
	}
	stats.dump();
}
