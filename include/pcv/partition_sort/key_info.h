#pragma once
#include <pcv/common/range.h>

namespace pcv {

template<typename key_t, typename index_t, typename rule_value_t>
class _KeyInfo {
public:
	Range1d<key_t> key;
	rule_value_t value;
	index_t next_level;

	_KeyInfo(Range1d<key_t> key_, rule_value_t value_, index_t next_level_) :
			key(key_), value(value_), next_level(next_level_) {
	}

	bool operator<(const _KeyInfo & other) const {
		return key < other.key;
	}
	bool operator<(const Range1d<key_t> & other_key) const {
		return key < other_key;
	}
	bool operator<(const key_t & other_key) const {
		return key < other_key;
	}
	bool operator>(const _KeyInfo & other) const {
		return key > other.key;
	}
	bool operator>(const Range1d<key_t> & other_key) const {
		return key > other_key;
	}
};

}
