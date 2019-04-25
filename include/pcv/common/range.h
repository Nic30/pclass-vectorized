#pragma once

#include <string>
#include <assert.h>
#include <limits>
#include <byteswap.h>
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

template<typename T>
class Range1d {
public:
	T low;
	T high;
	Range1d() :
			low(0), high(0) {
	}
	Range1d(T low, T high) :
			low(low), high(high) {
		assert(low <= high);
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

	/*
	 * Extract mask from the rule
	 **/
	T get_mask_littleendian() const {
		// [TODO] highly sub optimal use std::mismatch
		T msb = 1;
		msb <<= (sizeof(T) * 8 - 1);

		T m = 0;
		T last_mask = m;
		for (size_t i = 0; i < sizeof(T) * 8; i++) {
			m >>= 1;
			m |= msb;
			if ((low & m) != (high & m)) {
				break;
			}
			last_mask = m;
		}
		return last_mask;
	}
	T get_mask_bigendian() const {
		return endianity_swap::to_be(to_be().get_mask_littleendian());
	}
	Range1d to_be() const {
		return Range1d(endianity_swap::to_be(low), endianity_swap::to_be(high));
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

};

}
