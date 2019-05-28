#pragma once
#include <pcv/common/range.h>

namespace pcv {

template<typename value_t, typename index_t>
class _KeyInfo {
public:
	Range1d<value_t> key;
	index_t value;
	index_t next_level;

	_KeyInfo(Range1d<value_t> key_, index_t value_, index_t next_level_) :
			key(key_), value(value_), next_level(next_level_) {
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
};

}
