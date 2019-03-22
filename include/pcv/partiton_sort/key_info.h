#pragma once
#include <pcv/common/range.h>

namespace pcv {

template<typename T, typename index_t, typename value_t>
class _KeyInfo {
public:
	Range1d<T> key;
	index_t value;
	index_t next_level;

	_KeyInfo(Range1d<T> key, index_t value, index_t next_level) :
			key(key), value(value), next_level(next_level) {
	}

	bool operator<(const _KeyInfo & other) const {
		return key < other.key;
	}
	bool operator<(const Range1d<T> & other_key) const {
		return key < other_key;
	}
	bool operator<(const T & other_key) const {
		return key < other_key;
	}
	bool operator>(const _KeyInfo & other) const {
		return key > other.key;
	}
	bool operator>(const Range1d<T> & other_key) const {
		return key > other_key;
	}

	bool in_range(value_t val) const {
		return val >= key.low and val <= key.high;
	}

	//bool is_overlaping(const Range1d<T> & other) {
	//}
};
}
