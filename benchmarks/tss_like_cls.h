#pragma once
#include <array>
#include <vector>
#include <unordered_map>

#include <pcv/common/range.h>

class TSS_like {
public:
	struct rule_spec_t {
		std::array<pcv::Range1d<uint16_t>, 2> filter;
		size_t id;
	};
	// value : rule_id
	std::array<std::unordered_map<uint32_t, size_t>, 33> tables;
	// vector<table, mask>
	std::vector<std::pair<std::unordered_map<uint32_t, size_t>*, uint32_t>> used_tables;

	void insert(const rule_spec_t & r) {
		auto & f = r.filter;
		pcv::Range1d<uint32_t> v(((uint32_t) f[0].low) << 16 | f[1].low,
				((uint32_t) f[0].high) << 16 | f[1].high);
		auto pl = v.prefix_len_le();
		assert(pl <= 32);
		auto & t = tables.at(pl);
		t[v.low] = r.id;

		if (t.size() == 1) {
			// was newly added regenerate used_tables vector
			used_tables.clear();
			size_t i = 0;
			for (auto t_it = tables.rbegin(); t_it != tables.rend(); ++t_it) {
				if (t_it->size()) {
					uint32_t mask;
					if (i == 32)
						mask = -1;
					else {
						mask = (1 << i) - 1;
						mask = ~mask;
					}
					used_tables.push_back( { &*t_it, mask });
				}
				i++;
			}
		}
	}

	int search(std::array<uint16_t, 2> val) {
		uint32_t v = val[1];
		v <<= 16;
		v |= val[0];

		for (auto & t : used_tables) {
			auto res = t.first->find(v & t.second);
			if (res != t.first->end()) {
				return res->second;
			}
		}
		return -1;
	}
};
