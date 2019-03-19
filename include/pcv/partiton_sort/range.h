#pragma once

namespace pcv {

template<typename T>
class Range1d {
public:
	T low, high;
	Range1d(T low, T high) :
			low(low), high(high) {
	}
	/*
	 * @note overlap not checked
	 * */
	bool operator<(const Range1d & other) const {
		return high <= other.low;
	}
	bool operator<(const T & other) const {
		return high <= other;
	}
	bool operator>(const Range1d & other) const {
		return low >= other.high;
	}
	bool operator==(const Range1d & other) const {
		return low == other.low and high == other.high;
	}

};

}
