#pragma once

#include <vector>
#include <iostream>
#include <string>
#include <pcv/rule_parser/rule.h>

namespace pcv {

/*
 * The class which parser ClassBench or MSU to classes from pcv/rule_parser/rule.h
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
	std::vector<Rule_OF_1_5_1*> parse_openflow(const std::string& filename);

	void parse_rules(std::ifstream& fp, std::vector<iParsedRule*>& ruleset);
};

/*
 * @return vector<pair<rule, priority>>
 * */
std::vector<std::pair<iParsedRule*, size_t>> parse_ruleset_file(
		const std::string & rule_file);

}
