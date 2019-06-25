#pragma once
#include <unordered_map>
#include <map>
#include <stdint.h>
#include <vector>
#include <array>
#include <pcv/common/range.h>

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
		std::array<pcv::Range1d<uint16_t>, 2> filter;
		size_t id;
	};
	struct pair_hash {
		template<class T1, class T2>
		std::size_t operator()(const std::pair<T1, T2> &pair) const {
			return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
		}
	};
	// any in range to start of range
	std::unordered_map<uint16_t, uint16_t> data[2];
	std::unordered_map<std::pair<uint16_t, uint16_t>, priority_and_id, pair_hash> cross_product;
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

	int search(std::array<uint16_t, 2> val) {
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
