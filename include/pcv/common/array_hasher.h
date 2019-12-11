#pragma once

#include <array>
#include <boost/functional/hash.hpp>

namespace pcv {

/*
 * The hasher and comparator for std::array
 * */

template<typename ARR_T>
struct array_hasher {
	std::size_t operator()(const ARR_T &arr) const {
		using boost::hash_value;
		using boost::hash_combine;
		std::size_t seed = 0;
		for (auto v : arr) {
			hash_combine(seed, hash_value(v));
		}
		return seed;
	}
};

template<typename ARR_T>
struct array_eq {
	bool operator()(const ARR_T &a0, const ARR_T &a1) const {
		auto a1_it = a1.begin();
		for (auto v0 : a0) {
			if (v0 != *a1_it) {
				return false;
			}
			++a1_it;
		}
		return true;
	}
};

}
