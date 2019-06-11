#pragma once

#include <string>
#include <assert.h>
#include <limits>
#include <byteswap.h>
#include <tuple>
#include <linux/byteorder/little_endian.h>

namespace pcv {

namespace endianity_swap {

inline uint16_t to_be(uint16_t val) {
	return __cpu_to_be16(val);
}
inline uint32_t to_be(uint32_t val) {
	return __cpu_to_be32(val);
}
inline uint64_t to_be(uint64_t val) {
	return __cpu_to_be64(val);
}

}

template<typename _T>
class Range1d {
	// https://stackoverflow.com/questions/3313909/finding-the-length-of-the-common-prefix-in-two-bytes
	static int bytePrefix[256];
	static uint8_t mask_val[9];
	static std::pair<int, uint8_t> get_common_prefix_length(uint8_t a,
			uint8_t b) {
		auto pl = bytePrefix[a ^ b];
		return {pl, mask_val[pl]};
	}

public:
	using T = _T;
	T low;
	T high;
	Range1d() :
			low(0), high(0) {
	}
	Range1d(T low_, T high_, bool _be = false) :
			low(low_), high(high_) {
		if (! _be ) {
			assert(low <= high);
		}
	}
	static Range1d from_mask(T low, T mask) {
		return Range1d(low, low | ~mask);
	}
	/*
	 * @note overlap not checked
	 * */
	bool operator<(const Range1d & other) const {
		return low < other.low;
	}
	bool operator<(const T & other) const {
		return low < other;
	}
	bool operator>(const Range1d & other) const {
		return low > other.low;
	}
	bool operator==(const Range1d & other) const {
		return low == other.low and high == other.high;
	}
	bool operator!=(const Range1d & other) const {
		return low != other.low or high != other.high;
	}

	operator std::string() const {
		return std::to_string(low) + ":" + std::to_string(high);
	}

	bool overlaps(const Range1d & other) const {
		return (low <= other.low and high >= other.low)
				or (other.low <= low and other.high >= high);
	}

	T get_mask_le() const {
		return _get_mask_le().first;
	}
	T get_mask_be() const {
		return _get_mask_be().first;
	}
	/*
	 * Extract mask from the rule
	 **/
	std::pair<T, size_t> _get_mask_le() const {
		// [TODO] sub optimal use std::mismatch
		T mask = 0;
		size_t prefix_len = 0;
		auto m = reinterpret_cast<uint8_t *>(&mask);
		auto l = reinterpret_cast<const uint8_t *>(&low);
		auto h = reinterpret_cast<const uint8_t *>(&high);
		for (int b = sizeof(T) - 1; b >= 0; b--) {
			int pl;
			uint8_t pm;
			std::tie(pl, pm) = get_common_prefix_length(l[b], h[b]);
			m[b] = pm;
			prefix_len += pl;
			if (pl != 8) {
				break;
			}

		}
		return {mask, prefix_len};
	}
	std::pair<T, size_t> _get_mask_be() const {
		// [TODO] sub optimal use std::mismatch
		T mask = 0;
		size_t prefix_len = 0;
		auto m = reinterpret_cast<uint8_t *>(&mask);
		auto l = reinterpret_cast<const uint8_t *>(&low);
		auto h = reinterpret_cast<const uint8_t *>(&high);
		for (size_t b = 0; b < sizeof(T); b++) {
			int pl;
			uint8_t pm;
			std::tie(pl, pm) = get_common_prefix_length(l[b], h[b]);
			m[b] = pm;
			prefix_len += pl;
			if (pl != 8) {
				break;
			}

		}
		return {mask, prefix_len};
	}
	Range1d to_be() const {
		return Range1d(endianity_swap::to_be(low), endianity_swap::to_be(high), true);
	}
	bool is_wildcard() const {
		return low == 0 and high == std::numeric_limits<T>::max();
	}
	bool in_range(T val) const {
		return val >= low and val <= high;
	}
	void set_wildcard() {
		low = 0;
		high = std::numeric_limits<T>::max();
	}
	size_t prefix_len_le() const {
		return _get_mask_le().second;
	}
	size_t prefix_len_be() const {
		return to_be().prefix_len_le();
	}
	size_t max_prefix_len() {
		return sizeof(T) * 8;
	}
};

template<typename T>
int Range1d<T>::bytePrefix[256] = { 8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4,
		4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0 };
template<typename T>
uint8_t Range1d<T>::mask_val[9] = { 0, 0b10000000, 0b11000000, 0b11100000, 0xf0,
		0b11111000, 0b11111100, 0b11111110, 0xff };
}
