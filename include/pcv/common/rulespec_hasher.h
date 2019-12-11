#pragma once
#include <boost/functional/hash.hpp>

namespace pcv {
/*
 * The hasher and comparator for definition of match rules for packet classification
 * */

template<typename rule_spec_t>
struct rule_spec_t_eq {
	std::size_t operator()(const rule_spec_t &a, const rule_spec_t &b) const {
		if (a.second.rule_id != b.second.rule_id
				|| a.second.priority != b.second.priority)
			return false;
		auto _b = b.first.begin();
		for (auto v : a.first) {
			if (v != *_b)
				return false;
			++_b;
		}
		return true;
	}
};

template<typename rule_spec_t>
struct rule_spec_t_hasher {
	std::size_t operator()(const rule_spec_t &k) const {
		using boost::hash_value;
		using boost::hash_combine;

		std::size_t seed = 0;
		for (auto v : k.first) {
			hash_combine(seed, hash_value(v.low));
			hash_combine(seed, hash_value(v.high));
		}
		hash_combine(seed, hash_value(k.second.priority));
		hash_combine(seed, hash_value(k.second.rule_id));

		// Return the result.
		return seed;
	}
};

}
