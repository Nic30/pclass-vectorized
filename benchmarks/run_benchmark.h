#pragma once

#include <vector>
#include <array>
#include <pcv/common/range.h>
#include <pcv/utils/benchmark_common.h>

template<class CLS_T>
void run_benchmark_lookup_int(CLS_T & cls, pcv::BenchmarkStats & stats,
		const std::vector<typename CLS_T::key_vec_t> & packets,
		size_t LOOKUP_CNT) {
	typename CLS_T::rule_id_t res = 0;
	stats.lookup_start();
	// [TODO] assert enough large rule_cnt so cache is flushed
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto p_id = i % packets.size();
		auto & p = packets[p_id];
		stats.lookup_packet_start();
		res |= cls.search(p);
		stats.lookup_packet_stop(p_id);
	}
	// this is there to assert the search is not optimised out
	if (res == 0) {
		throw std::runtime_error("probably wrong result");
	}
	stats.lookup_stop();
}

template<class CLS_T>
void run_benchmark_lookup_ptr(CLS_T & cls, pcv::BenchmarkStats & stats,
		const std::vector<typename CLS_T::key_vec_t> & packets,
		size_t LOOKUP_CNT) {
	typename CLS_T::rule_id_t res = nullptr;
	stats.lookup_start();
	// [TODO] assert enough large rule_cnt so cache is flushed
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto p_id = i % packets.size();
		auto & p = packets[p_id];
		stats.lookup_packet_start();
		res = reinterpret_cast<typename CLS_T::rule_id_t>(uintptr_t(res)
				+ uintptr_t(cls.search(p)));
		stats.lookup_packet_stop(p_id);
	}
	// this is there to assert the search is not optimised out
	if (res == nullptr) {
		throw std::runtime_error("probably wrong result");
	}
	stats.lookup_stop();
}

template<class CLS_T>
void run_benchmark_lookup_struct(CLS_T & cls, pcv::BenchmarkStats & stats,
		const std::vector<typename CLS_T::key_vec_t> & packets,
		size_t LOOKUP_CNT) {
	typename CLS_T::rule_value_t res;
	stats.lookup_start();
	// [TODO] assert enough large rule_cnt so cache is flushed
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto p_id = i % packets.size();
		auto & p = packets[p_id];
		stats.lookup_packet_start();
		res.rule_id |= cls.search(p).rule_id;
		stats.lookup_packet_stop(p_id);
	}
	// this is there to assert the search is not optimised out
	if (res.rule_id == 0) {
		throw std::runtime_error("probably wrong result");
	}
	stats.lookup_stop();
}

template<class CLS_T>
void run_classfier_init(CLS_T & cls, pcv::BenchmarkStats & stats,
		const std::vector<std::array<pcv::Range1d<uint16_t>, 2>> & rules) {
	stats.construction_start();
	typename CLS_T::rule_id_t i = 0;
	for (auto & r : rules) {
		typename CLS_T::rule_value_t k;
		k.priority = 0;
		k.rule_id = i;
		cls.insert( { r, k });
		i++;
	}
	stats.construction_stop();
}

template<class CLS_T>
void run_benchmark_struct(CLS_T & cls,
		const std::vector<std::array<pcv::Range1d<uint16_t>, 2>> & rules,
		const std::vector<typename CLS_T::key_vec_t> & packets,
		size_t LOOKUP_CNT) {
	pcv::BenchmarkStats stats(LOOKUP_CNT, rules.size(), packets.size());
	run_classfier_init(cls, stats, rules);
	run_benchmark_lookup_struct(cls, stats, packets, LOOKUP_CNT);
	stats.dump();
}

template<class CLS_T>
void run_benchmark_int(CLS_T & cls,
		const std::vector<std::array<pcv::Range1d<uint16_t>, 2>> & rules,
		const std::vector<typename CLS_T::key_vec_t> & packets,
		size_t LOOKUP_CNT) {
	pcv::BenchmarkStats stats(LOOKUP_CNT, rules.size(), packets.size());
	run_classfier_init(cls, stats, rules);
	run_benchmark_lookup_int(cls, stats, packets, LOOKUP_CNT);
	stats.dump();
}

