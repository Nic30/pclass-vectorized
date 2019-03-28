#pragma once

#include <string>
#include <assert.h>
#include <limits>

namespace pcv {

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

	bool is_wildcard() const {
		return low == 0 and std::numeric_limits<T>::max();
	}
	bool in_range(T val) const {
		return val >= low and val <= high;
	}

};

}
