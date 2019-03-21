#pragma once

#include <string>

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

	operator std::string() {
		return std::to_string(low) + ":" + std::to_string(high);
	}

};

}
