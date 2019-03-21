#include <pcv/rule_parser/classbench_rule_parser.h>

#include <vector>
#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <string>
#include <fstream>
#include <sstream>
#include <limits>
#include <iterator>
#include <functional>
#include <regex>

using namespace std;
namespace pcv {

Rule_Ipv4::Rule_Ipv4() :
		sip(0, std::numeric_limits < uint32_t > ::max()), dip(0,
				std::numeric_limits < uint32_t > ::max()), sport(0,
				std::numeric_limits < uint16_t > ::max()), dport(0,
				std::numeric_limits < uint16_t > ::max()), proto(0,
				std::numeric_limits < uint16_t > ::max()) {

}

RuleReader::RuleReader() :
		dim(5), reps(1) {
}

unsigned int inline RuleReader::atoui(const string& in) {
	std::istringstream reader(in);
	unsigned int val;
	reader >> val;
	return val;
}

std::vector<std::string> & RuleReader::split(const std::string &s, char delim,
		std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> RuleReader::split(const std::string &s, char delim) {
	std::vector < std::string > elems;
	split(s, delim, elems);
	return elems;
}

vector<vector<unsigned int>> RuleReader::parse_packets(const string& filename) {
	vector<vector<unsigned int>> packets;
	ifstream input_file(filename);
	if (!input_file.is_open()) {
		throw std::runtime_error(
				string("Couldnt open packet set file ") + filename);
	}
	int line_number = 1;
	string content;
	while (getline(input_file, content)) {
		istringstream iss(content);
		vector < string > tokens { istream_iterator<string> { iss },
				istream_iterator<string> { } };
		vector<unsigned int> one_packet;
		for (int i = 0; i < dim; i++) {
			one_packet.push_back(atoui(tokens[i]));
		}
		packets.push_back(one_packet);
		line_number++;

	}
	return packets;
}

void RuleReader::parse_IPRange(Range1d<uint32_t>& IPrange,
		const string& token) {
	//split slash
	vector < string > split_slash = split(token, '/');
	vector < string > split_ip = split(split_slash[0], '.');
	// asindmemacces IPv4 prefixes
	// temporary variables to store IP range
	unsigned int mask;
	int masklit1;
	unsigned int masklit2, masklit3;
	unsigned int ptrange[4];
	for (int i = 0; i < 4; i++)
		ptrange[i] = atoui(split_ip[i]);
	mask = atoui(split_slash[1]);

	mask = 32 - mask;
	masklit1 = mask / 8;
	masklit2 = mask % 8;

	/*count the start IP */
	for (int i = 3; i > 3 - masklit1; i--)
		ptrange[i] = 0;
	if (masklit2 != 0) {
		masklit3 = 1;
		masklit3 <<= masklit2;
		masklit3 -= 1;
		masklit3 = ~masklit3;
		ptrange[3 - masklit1] &= masklit3;
	}
	/*store start IP */
	IPrange.low = ptrange[0];
	IPrange.low <<= 8;
	IPrange.low += ptrange[1];
	IPrange.low <<= 8;
	IPrange.low += ptrange[2];
	IPrange.low <<= 8;
	IPrange.low += ptrange[3];

	//key += std::bitset<32>(IPrange[0] >> prefix_length).to_string().substr(32 - prefix_length);
	/*count the end IP*/
	for (int i = 3; i > 3 - masklit1; i--)
		ptrange[i] = 255;
	if (masklit2 != 0) {
		masklit3 = 1;
		masklit3 <<= masklit2;
		masklit3 -= 1;
		ptrange[3 - masklit1] |= masklit3;
	}
	/*store end IP*/
	IPrange.high = ptrange[0];
	IPrange.high <<= 8;
	IPrange.high += ptrange[1];
	IPrange.high <<= 8;
	IPrange.high += ptrange[2];
	IPrange.high <<= 8;
	IPrange.high += ptrange[3];
}
void RuleReader::parse_port(Range1d<uint16_t>& Portrange, const string& from,
		const string& to) {
	Portrange.low = atoui(from);
	Portrange.high = atoui(to);
}

void RuleReader::parse_protocol(Range1d<uint16_t>& Protocol,
		const string& last_token) {
	// Example : 0x06/0xFF
	vector < string > split_slash = split(last_token, '/');

	if (split_slash[1] != "0xFF") {
		Protocol.low = 0;
		Protocol.high = 255;
	} else {
		Protocol.low = Protocol.high = std::stoul(split_slash[0], nullptr, 16);
	}
}

void RuleReader::parse(vector<string>& tokens, vector<iParsedRule*>& ruleset,
		unsigned int cost) {
	// 5 fields: sip, dip, sport, dport, proto = 0 (with@), 1, 2 : 4, 5 : 7, 8

	/*allocate a few more bytes just to be on the safe side to avoid overflow etc*/
	auto temp_rule = new Rule_Ipv4;
	string key;
	if (tokens[0].at(0) != '@') {
		/* each rule should begin with an '@' */
		throw runtime_error(string("not a valid rule format") + tokens[0]);
	}

	int index_token = 0;
	/* reading SIP range */
	// skipping the initial char
	parse_IPRange(temp_rule->sip, tokens[index_token++].substr(1));

	/* reading DIP range */
	parse_IPRange(temp_rule->dip, tokens[index_token++]);
	parse_port(temp_rule->sport, tokens[index_token], tokens[index_token + 2]);
	index_token += 3;
	parse_port(temp_rule->dport, tokens[index_token], tokens[index_token + 2]);
	index_token += 3;
	parse_protocol(temp_rule->proto, tokens[index_token++]);

	temp_rule->priority = cost;

	ruleset.push_back(temp_rule);
}
void RuleReader::parse_rules(ifstream& fp, vector<iParsedRule*>& ruleset) {
	int line_number = 0;
	string content;
	while (getline(fp, content)) {
		istringstream iss(content);
		vector < string > tokens { istream_iterator<string> { iss },
				istream_iterator<string> { } };
		parse(tokens, ruleset, line_number++);
	}
}
vector<iParsedRule*> RuleReader::parse_classbench(const string& filename) {
	//assume 5*rep fields

	vector<iParsedRule*> rules;
	ifstream column_counter(filename);
	ifstream input_file(filename);
	if (!input_file.is_open() || !column_counter.is_open()) {
		throw ifstream("Couldnt open filter set file");
	}

	parse_rules(input_file, rules);
	input_file.close();
	column_counter.close();

	//need to rearrange the priority

	int max_pri = rules.size() - 1;
	for (size_t i = 0; i < rules.size(); i++) {
		rules[i]->priority = max_pri - i;
	}

	return rules;
}

bool IsPower2(unsigned int x) {
	return ((x - 1) & x) == 0;
}

bool IsPrefix(unsigned int low, unsigned int high) {
	unsigned int diff = high - low;
	return ((low & high) == low) && IsPower2(diff + 1);
}

unsigned int PrefixLength(unsigned int low, unsigned int high) {
	unsigned int x = high - low;
	int lg = 0;
	for (; x; x >>= 1)
		lg++;
	return 32 - lg;
}

void RuleReader::parse_range(Range1d<uint32_t>& range, const string& text) {
	vector < string > split_colon = split(text, ':');
	// to obtain interval
	range.low = atoui(split_colon[0]);
	range.high = atoui(split_colon[1]);
	if (range.low > range.high) {
		stringstream ss;
		ss << "Problematic range:" << range.low << "-" << range.high;
		throw runtime_error(ss.str());
	}
}

vector<iParsedRule*> RuleReader::parser_MSU(const string& filename) {
	vector<iParsedRule*> rules;
	ifstream input_file(filename);
	if (!input_file.is_open()) {
		throw runtime_error(string("Couldnt open filter set file ") + filename);
	}
	string content;
	getline(input_file, content);
	getline(input_file, content);
	vector < string > split_comma = split(content, ',');
	dim = split_comma.size();

	int priority = 0;
	getline(input_file, content);
	vector < string > parts = split(content, ',');
	vector < Range1d < uint32_t >> bounds(parts.size());
	for (size_t i = 0; i < parts.size(); i++) {
		parse_range(bounds[i], parts[i]);
	}

	while (getline(input_file, content)) {
		// 5 fields: sip, dip, sport, dport, proto = 0 (with@), 1, 2 : 4, 5 : 7, 8
		auto temp_rule = new Rule_Ipv4;
		vector < string > split_comma = split(content, ',');
		// ignore priority at the end
		for (size_t i = 0; i < split_comma.size() - 1; i++) {
			parse_range(temp_rule->sip, split_comma[i]);
		}
		temp_rule->priority = priority++;
		temp_rule->tag = atoi(split_comma[split_comma.size() - 1].c_str());
		rules.push_back(temp_rule);
	}
	for (auto & r : rules) {
		r->priority = rules.size() - r->priority;
	}

	return rules;
}

vector<iParsedRule*> RuleReader::parse_rules(const string& filename) {
	vector<iParsedRule*> res;
	ifstream in(filename);
	if (!in.is_open()) {
		throw ifstream::failure(
				string("Couldnt open filter set file \"") + filename + "\"");
	}

	string content;
	getline(in, content);
	istringstream iss(content);
	vector < string > tokens { istream_iterator<string> { iss },
			istream_iterator<string> { } };
	if (content.length() == 0) {
		in.close();
		throw ifstream::failure("File is empty");
	} else if (content[0] == '!') {
		// MSU FORMAT
		vector < string > split_semi = split(tokens.back(), ';');
		reps = (atoi(split_semi.back().c_str()) + 1) / 5;
		dim = reps * 5;

		res = parser_MSU(filename);
	} else if (content[0] == '@') {
		// CLassBench Format
		/* COUNT COLUMN */

		if (tokens.size() % 9 == 0) {
			reps = tokens.size() / 9;
		}

		dim = reps * 5;
		res = parse_classbench(filename);
	} else {
		in.close();
		throw ifstream::failure(
				string(
						"unknown input format please use either MSU format or ClassBench format (content[0]=='")
						+ content[0] + "')");
	}
	in.close();
	return res;
}

}
