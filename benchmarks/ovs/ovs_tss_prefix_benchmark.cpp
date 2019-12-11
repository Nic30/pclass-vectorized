#include <vector>
#include <unordered_map>
#include <map>
#include <stdint.h>
#include <random>
#include <iostream>
#include <linux/swab.h>
#include <functional>
#include <assert.h>
#include <stdexcept>
#include "lib/pvector.h"
#include "lib/classifier.h"

#include <pcv/utils/benchmark_common.h>

#undef atomic_store
#undef atomic_compare_exchange_strong_explicit
#undef atomic_compare_exchange_strong
#undef atomic_compare_exchange_weak_explicit
#undef atomic_compare_exchange_weak

using namespace std;

/*
 * Benchmark which uses multiple rules with different prefix length to test OvS classifier properties.
 **/
int main(int argc, const char *argv[]) {
	assert(argc == 1 + 4);
	string CLS_NAME = argv[1];
	size_t PREFIX_CNT = atoll(argv[2]);
	size_t UNIQUE_TRACE_CNT = atoll(argv[3]);
	size_t LOOKUP_CNT = atoll(argv[4]);

	classifier cls;
	classifier_init(&cls, nullptr);
	pcv::BenchmarkStats stats(LOOKUP_CNT, PREFIX_CNT, PREFIX_CNT);
	int version = 0;
	stats.construction_start();
	for (size_t i = 1; i < PREFIX_CNT + 1; i++) {
		struct match match;
		match_init_catchall(&match);

		if (i < 32) {
			uint32_t v = 1 << (i - 1);
			uint32_t m = (1 << i) - 1;

			match.flow.nw_src = v;
			match.wc.masks.nw_src = m;
		} else {
			uint32_t v = 1 << ((i % 32));
			uint32_t m = (1 << (i % 32)) - 1;
			match.flow.nw_src = -1;
			match.wc.masks.nw_src = -1;
			match.flow.nw_dst = v;
			match.wc.masks.nw_dst = m;
		}

		//std::cout << v << " " << m << std::endl;
		// match.flow.nw_src = __swab32(v);
		// match.wc.masks.nw_src = __swab32(m);

		struct cls_rule *rule = (struct cls_rule*) xzalloc(sizeof *rule);

		int priority = i;
		cls_rule_init(rule, &match, priority);
		auto existing_rule = classifier_find_rule_exactly(&cls, rule, version);
		if (existing_rule) {
			std::cout << " insert failed because rule exists" << std::endl;
			free(rule);
			return false;
		} else {
			classifier_insert(&cls, rule, version, nullptr, 0);
		}
		// struct flow f;
		// memset(&f, 0, sizeof f);
		// f.nw_src = 1 << (i - 1);
		// auto r = classifier_lookup(&cls, version, &f, nullptr);
		// if (r != rule) {
		// 	throw std::runtime_error("invalid match");
		// }
	}
	stats.construction_stop();

	auto packets = new struct flow[PREFIX_CNT];
	for (size_t i = 1; i < PREFIX_CNT + 1; i++) {
		auto &p = packets[i - 1];
		memset(&p, 0, sizeof p);
		if (i < 32) {
			p.nw_src = 1 << (i - 1);
		} else {
			p.nw_src = -1;
			p.nw_src = 1 << ((i % 32));
		}
	}

	std::mt19937 rand(0);
	auto dice_pi = std::bind(
			std::uniform_int_distribution<int>(0, PREFIX_CNT - 1), rand);

	stats.lookup_start();
	//stats.lookup_packet_start();
	for (size_t i = 0; i < LOOKUP_CNT; i++) {
		auto pi = dice_pi();
		auto p = &packets[pi];
		auto r = classifier_lookup(&cls, version, p, nullptr);
		// std::cout << pi << " " << r << " " << p->nw_src << std::endl;
		assert(r);
	}
	stats.lookup_stop();

	stats.dump();
	return 0;
}
