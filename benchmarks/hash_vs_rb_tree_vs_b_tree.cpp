#include <vector>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include "run_benchmark.h"
#include "hash_table_cls.h"

#include <unordered_map>
#include <map>
#include <stdint.h>

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

using BTree = BTreeImp<_BTreeCfg<uint16_t, IntRuleValue, 2, (1<<16) - 1, 4, false>>;
//using BTree_compressed = BTreeImp<_BTreeCfg<uint16_t, IntRuleValue, 2, (1<<16) - 1, 4, true>>;


int main(int argc, const char * argv[]) {
	assert(argc == 1 + 4);
	string CLS_NAME = argv[1];
	size_t RULE_CNT = atoll(argv[2]);
	size_t UNIQUE_TRACE_CNT = atoll(argv[3]);
	size_t LOOKUP_CNT = atoll(argv[4]);

	std::vector<array<Range1d<uint16_t>, 2>> rules;
	rules.reserve(RULE_CNT);
	// generate sequence of /24 addresses
	for (uint32_t i = 0; i < RULE_CNT; i++) {
		uint32_t v = i;
		v <<= 8;
		uint16_t l = v;
		uint16_t h = v >> 16;
		rules.push_back(
				{ Range1d<uint16_t>(h, h), Range1d<uint16_t>(l, l + 0xff) });
	}

	vector<array<uint16_t, 2>> packets;
	{
		std::mt19937 gen(0);
		std::uniform_int_distribution<uint32_t> dis(0, RULE_CNT << 8);
		for (size_t i = 0; i < UNIQUE_TRACE_CNT; i++) {
			uint32_t v = dis(gen);
			packets.push_back( { (uint16_t) (v >> 16), (uint16_t) v });
		}
	}

	if (CLS_NAME == "hash") {
		HashTableBasedCls cls;
		run_benchmark_struct(cls, rules, packets, LOOKUP_CNT);
	} else if (CLS_NAME == "b_tree") {
		BTree cls;
		run_benchmark_struct(cls, rules, packets, LOOKUP_CNT);
	} else {
		throw runtime_error("Unsupported classifier");
	}
}
