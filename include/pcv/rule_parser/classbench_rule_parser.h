#pragma once

#include <vector>
#include <array>
#include <iostream>
#include <string>
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

template<>
std::array<Range1d<uint16_t>, 7> rule_to_array(const Rule_Ipv4 & r) {
	std::array<Range1d<uint16_t>, 7> _r;
	using R = Range1d<uint16_t>;
	auto m = std::numeric_limits<uint16_t>::max();
	_r[0] = R((r.sip.low >> 16) & m, (r.sip.high >> 16) & m);
	_r[1] = R(r.sip.low & m, r.sip.high & m);
	_r[2] = R((r.dip.low >> 16) & m, (r.dip.high >> 16) & m);
	_r[3] = R(r.dip.low & m, r.dip.high & m);
	_r[4] = r.sport;
	_r[5] = r.dport;
	_r[6] = r.proto;

	return _r;
}

}

/*
 *
 **/
class RuleReader {
public:
	int dim;
	int reps;
	RuleReader();
	std::vector<iParsedRule*> parse_rules(const std::string& filename);

	std::vector<std::vector<unsigned int>> parse_packets(
			const std::string& filename);
private:
	unsigned int inline atoui(const std::string& in);
	std::vector<std::string> &split(const std::string &s, char delim,
			std::vector<std::string> &elems);
	std::vector<std::string> split(const std::string &s, char delim);

	void parse_IPRange(Range1d<uint32_t> & IPrange, const std::string& token);
	void parse_port(Range1d<uint16_t> & Portrange, const std::string& from,
			const std::string& to);
	void parse_protocol(Range1d<uint16_t>& Protocol,
			const std::string& last_token);
	void parse_range(Range1d<uint32_t>& range, const std::string& text);

	void parse(std::vector<std::string>& tokens,
			std::vector<iParsedRule*>& ruleset, unsigned int cost);

	std::vector<iParsedRule*> parse_classbench(const std::string& filename);
	std::vector<iParsedRule*> parser_MSU(const std::string& filename);
	void parse_rules(std::ifstream& fp, std::vector<iParsedRule*>& ruleset);
};

}
