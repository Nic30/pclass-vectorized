#include <vector>
#include <pcv/partition_sort/b_tree_impl.h>
#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/utils/benchmark_common.h>

#include <unordered_map>
#include <map>
#include <stdint.h>

using namespace std;
using namespace pcv;
using namespace pcv::rule_conv_fn;

class HashTableBasedCls {
public:
	struct priority_and_id {
		int priority;
		size_t id;

		priority_and_id() :
				priority(-1), id(-1) {
		}
		priority_and_id(int _priority, size_t _id) :
				priority(_priority), id(_id) {
		}
	};
	struct rule_spec_t {
		array<Range1d<uint16_t>, 2> filter;
		size_t id;
	};
	struct pair_hash {
		template<class T1, class T2>
		std::size_t operator()(const std::pair<T1, T2> &pair) const {
			return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
		}
	};
	// any in range to start of range
	unordered_map<uint16_t, uint16_t> data[2];
	unordered_map<pair<uint16_t, uint16_t>, priority_and_id, pair_hash> cross_product;
	uint16_t data_2_next_id;
	HashTableBasedCls() :
			data_2_next_id(0) {
	}
	void insert(const rule_spec_t & r) {
		// for each colliding range check all records witch are using this range in cross production table
		// and update them
		auto f0 = r.filter[0];
		assert(f0.high == f0.low);
		auto _f0 = data[0].find(f0.low);
		if (_f0 != data[0].end())
			data[0][f0.low] = f0.low;

		auto f1 = r.filter[1];
		for (ssize_t l = f1.low; l <= (ssize_t) f1.high; l++) {
			auto _f1 = data[1].find(l);
			uint16_t f1_val;
			if (_f1 == data[1].end()) {
				f1_val = data_2_next_id;
				data_2_next_id++;
				data[1][l] = f1_val;
			} else {
				f1_val = _f1->second;
			}
			cross_product[ { f0.low, f1_val }] = priority_and_id(0, r.id);
		}
	}

	int search(array<uint16_t, 2> val) {
		auto k0 = data[0].find(val[0]);
		if (k0 == data[0].end())
			return -1;
		auto k1 = data[1].find(val[1]);
		if (k1 == data[1].end())
			return -1;
		auto res = cross_product.find( { k0->second, k1->second });
		if (res == cross_product.end())
			return -1;
		return res->second.id;
	}

};

using BTree = BTreeImp<uint16_t, 2, 4, false>;
//using BTree_compressed = BTreeImp<uint16_t, 2, 4, true>;

template<class CLS_T>
void run_benchmark(CLS_T & cls,
		const std::vector<array<Range1d<uint16_t>, 2>> & rules,
		const vector<array<uint16_t, 2>> & packets, size_t LOOKUP_CNT) {
	BenchmarkStats stats(LOOKUP_CNT, true, rules.size());
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
		run_benchmark(cls, rules, packets, LOOKUP_CNT);
	} else if (CLS_NAME == "b_tree") {
		BTree cls;
		run_benchmark(cls, rules, packets, LOOKUP_CNT);
	} else {
		throw runtime_error("Unsupported classifier");
	}
}
