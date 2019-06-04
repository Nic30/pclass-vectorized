#pragma once

#include <vector>
#include <algorithm>

#include <pcv/common/range.h>

namespace pcv {

template<typename _Key_t, size_t _D>
class ListBasedClassifier {
public:
	using value_t = _Key_t;
	using rule_id_t = uint16_t;
	static constexpr size_t D = _D;
	using val_range_t = Range1d<_Key_t>;
	using rule_spec_t = std::pair<std::array<val_range_t, D>, rule_id_t>;
	using val_vec_t = std::array<value_t, D>;
	static constexpr rule_id_t INVALID_RULE =
			std::numeric_limits<rule_id_t>::max();
	std::vector<rule_spec_t> rules;
	bool rules_sorted;
	ListBasedClassifier() :
			rules_sorted(true) {

	}
	inline void insert(const rule_spec_t & r) {
		rules.push_back(r);
		rules_sorted = false;
	}

	inline void prepare() {
		std::sort(rules.begin(), rules.end(),
				[](const rule_spec_t & a, const rule_spec_t & b) {
					return a.second > b.second;
				});
		rules_sorted = true;
	}

	rule_id_t search(const val_vec_t & v) const {
		if (not rules_sorted) {
			throw std::runtime_error("rules not prepared for the lookup");
		}
		for (auto & r : rules) {
			bool match = true;
			auto vIt = v.begin();
			auto rIt = r.first.begin();
			for (; rIt != r.first.end(); ++rIt, ++vIt) {
				if (!rIt->in_range(*vIt)) {
					match = false;
					break;
				}
			}
			if (match)
				return r.second;
		}
		return INVALID_RULE;
	}
	inline void remove(const rule_spec_t & r) {
		rules.erase(std::remove(rules.begin(), rules.end(), r), rules.end());
	}
};

}
