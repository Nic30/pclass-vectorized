#pragma once
#include <pcv/common/range.h>

namespace pcv {

template<typename value_t, typename index_t>
class _KeyInfo {
public:
	Range1d<value_t> key;
	index_t value;
	index_t next_level;

	_KeyInfo(Range1d<value_t> key, index_t value, index_t next_level) :
			key(key), value(value), next_level(next_level) {
	}

	bool operator<(const _KeyInfo & other) const {
		return key < other.key;
	}
	bool operator<(const Range1d<value_t> & other_key) const {
		return key < other_key;
	}
	bool operator<(const value_t & other_key) const {
		return key < other_key;
	}
	bool operator>(const _KeyInfo & other) const {
		return key > other.key;
	}
	bool operator>(const Range1d<value_t> & other_key) const {
		return key > other_key;
	}

	bool in_range(value_t val) const {
		return val >= key.low and val <= key.high;
	}
};

}
