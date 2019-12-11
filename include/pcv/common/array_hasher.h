#pragma once

#include <array>
#include <boost/functional/hash.hpp>

namespace pcv {

/*
 * The hasher and comparator for std::array
 * */

template<typename VAL_T, std::size_t D>
struct array_hasher {
	std::size_t operator()(const std::array<VAL_T, D> &arr) const {
		using boost::hash_value;
		using boost::hash_combine;
		std::size_t seed = 0;
		for (auto v : arr) {
			hash_combine(seed, hash_value(v));
		}
		return seed;
	}
};

template<typename VAL_T, std::size_t D>
struct array_eq {
	bool operator()(const std::array<VAL_T, D> &a0,
			const std::array<VAL_T, D> &a1) const {
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
