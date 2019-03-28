#pragma once
#include <stdint.h>
#include <array>
#include <pcv/common/range.h>

namespace pcv {

class iParsedRule {
public:
	int priority;
	int tag;
	uint16_t vlan;
	iParsedRule() :
			priority(-1), tag(-1), vlan(0) {
	}
};

class Rule_Ipv4: public iParsedRule {
public:
	// values stored in little-endian to be easily comparable on x86_64
	Range1d<uint32_t> sip;
	Range1d<uint32_t> dip;
	Range1d<uint16_t> sport;
	Range1d<uint16_t> dport;
	Range1d<uint16_t> proto;

	Rule_Ipv4();
	// serialize rule to classbench format
	friend std::ostream & operator<<(std::ostream & str, const Rule_Ipv4 & r);
	operator std::string() const;
};

class Rule_Ipv6: public iParsedRule {
public:

};

namespace rule_conv_fn {

template<typename T, size_t D>
std::array<Range1d<T>, D> rule_to_array(const Rule_Ipv4 & r);

}

}
