#include <vector>
#include <unordered_map>
#include <map>
#include <stdint.h>

#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include "run_benchmark.h"
#include "hash_table_cls.h"
#include "tss_like_cls.h"
#include "run_benchmark.h"

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

using BTree = BTreeImp<uint16_t, IntRuleValue, 2, 4, false>;
//using BTree_compressed = BTreeImp<uint16_t, IntRuleValue, 2, 4, true>;

int main(int argc, const char * argv[]) {
	assert(argc == 1 + 4);
	string CLS_NAME = argv[1];
	size_t PREFIX_CNT = atoll(argv[2]);
	size_t UNIQUE_TRACE_CNT = atoll(argv[3]);
	size_t LOOKUP_CNT = atoll(argv[4]);

	assert(PREFIX_CNT <= 32);
	std::vector<array<Range1d<uint16_t>, 2>> rules;
	rules.reserve(PREFIX_CNT);
	for (size_t i = 0; i < PREFIX_CNT; i++) {
		// i = 32 - prefix_len - 1 = length of non used bits
		uint32_t L = 1 << i;
		uint32_t H = L + (((uint64_t) 1) << i) - 1;
		// cout << hex << H << " " << hex << L << endl;

		uint16_t hh = H >> 16, hl = L >> 16, lh = H, ll = L;
		rules.push_back(
				{ Range1d<uint16_t>(hl, hh), Range1d<uint16_t>(ll, lh) });
	}

	vector<array<uint16_t, 2>> packets;
	{
		std::mt19937 gen(0);
		std::uniform_int_distribution<> dis(0, 32);
		for (size_t i = 0; i < UNIQUE_TRACE_CNT; i++) {
			int prefix_len = dis(gen);
			uint32_t v = 1 << (32 - prefix_len - 1);
			packets.push_back( { (uint16_t) (v >> 16), (uint16_t) v });
		}
	}

	if (CLS_NAME == "tss") {
		TSS_like cls;
		run_benchmark_struct(cls, rules, packets, LOOKUP_CNT);
		//} else if (CLS_NAME == "hash") {
		//	HashTableBasedCls cls;
		//	run_benchmark(cls, rules, packets, LOOKUP_CNT);
	} else if (CLS_NAME == "b_tree") {
		BTree cls;
		run_benchmark_struct(cls, rules, packets, LOOKUP_CNT);
	} else {
		throw runtime_error("Unsupported classifier");
	}
}
