#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/common/range.h>

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
#include <iomanip>

using namespace std;
namespace pcv {

Rule_Ipv4_ACL::Rule_Ipv4_ACL() :
		sip(0, std::numeric_limits<uint32_t>::max()), dip(0,
				std::numeric_limits<uint32_t>::max()), sport(0,
				std::numeric_limits<uint16_t>::max()), dport(0,
				std::numeric_limits<uint16_t>::max()), proto(0,
				std::numeric_limits<uint16_t>::max()) {

}

size_t Rule_Ipv4_ACL::cummulative_prefix_len() {
	return sip.prefix_len_le() + dip.prefix_len_le() + sport.prefix_len_le()
			+ dport.prefix_len_le() + proto.prefix_len_le();
}

size_t Rule_Ipv4_ACL::max_cummulative_prefix_len() {
	return sip.max_prefix_len() + dip.max_prefix_len() + sport.max_prefix_len()
			+ dport.max_prefix_len() + proto.max_prefix_len();
}

void Rule_Ipv4_ACL::reverse_endianity() {
	sip = sip.to_be();
	dip = dip.to_be();
	sport = sport.to_be();
	dport = dport.to_be();
	proto = proto.to_be();
}

std::ostream & print_ipv4(std::ostream & str, const Range1d<uint32_t> & ip) {
	size_t prefix_len = 32;
	while (prefix_len > 0) {
		auto m = 1 << (32 - prefix_len);
		if ((ip.low & m) == (ip.high & m)) {
			break;
		}
		prefix_len--;
	}
	auto bytes = reinterpret_cast<const uint8_t*>(&ip.low);
	for (int i = 3; i >= 0; i--) {
		str << int(bytes[i]);
		if (i != 0)
			str << ".";
	}
	str << "/" << prefix_len;
	return str;
}

std::ostream & print_range(std::ostream & str, const Range1d<uint16_t> & range,
		bool hex) {
	auto f = str.flags();
	if (hex) {
		size_t prefix_mask = 0xff;
		if (range.low != range.high)
			prefix_mask = 0;
		str << "0x" << std::hex << std::uppercase << std::setfill('0')
				<< std::setw(2) << range.low << "/0x" << std::hex
				<< std::uppercase << std::setfill('0') << std::setw(2)
				<< prefix_mask;
	} else {
		str << range.low << " : " << range.high;
	}
	str.flags(f);
	return str;
}

std::ostream & operator<<(std::ostream & str, const Rule_Ipv4_ACL & r) {
	// @42.38.199.209/32	111.195.251.32/32	0 : 65535	1521 : 1521	0x06/0xFF
	str << "@";
	print_ipv4(str, r.sip) << "\t";
	print_ipv4(str, r.dip) << "\t";
	print_range(str, r.sport, false) << "\t";
	print_range(str, r.dport, false) << "\t";
	print_range(str, r.proto, true);

	return str;
}

std::ostream & operator<<(std::ostream & str, const ipv6_t & r) {
	auto v = reinterpret_cast<const uint8_t*>(&r);
	auto f = str.flags();

	for (int i = 0; i < 16; i++) {
		str << std::hex << unsigned(v[i]);
		if (i != 15)
			str << ":";
	}
	str.flags(f);
	return str;
}

Rule_Ipv4_ACL::operator std::string() const {
	stringstream ss;
	ss << *this;
	return ss.str();
}

namespace OF_range_printer {
template<typename T>
bool print(std::ostream & str, const string & val_name, const Range1d<T> & val,
		bool something_was_before) {

	if (val.is_wildcard()) {
		return something_was_before;
	}
	if (not something_was_before) {
		str << " ";
	} else {
		str << ", ";
	}
	str << val_name << "=";
	if (val.high == val.low) {
		str << val.low;
	} else {
		auto f = str.flags();
		str << std::hex << "0x" << val.low << "/0x"
				<< val.get_mask_le();
		str.flags(f);
	}

	return true;
}
}

std::ostream & operator<<(std::ostream & str, const Rule_OF_1_5_1& r) {
	using namespace OF_range_printer;
	bool f = print(str, "in_port", r.in_port, false);
	f = print(str, "in_phy_port", r.in_phy_port, f);
	f = print(str, "metadata", r.metadata, f);
	f = print(str, "eth_dst", r.eth_dst, f);
	f = print(str, "eth_src", r.eth_src, f);
	f = print(str, "eth_type", r.eth_type, f);
	f = print(str, "vlan_vid", r.vlan_vid, f);
	f = print(str, "vlan_pcp", r.vlan_pcp, f);
	f = print(str, "ip_dscp", r.ip_dscp, f);
	f = print(str, "ip_ecn", r.ip_ecn, f);
	f = print(str, "ip_proto", r.ip_proto, f);
	f = print(str, "ipv4_src", r.ipv4_src, f);
	f = print(str, "ipv4_dst", r.ipv4_dst, f);
	f = print(str, "tcp_src", r.tcp_src, f);
	f = print(str, "tcp_dst", r.tcp_dst, f);
	f = print(str, "udp_src", r.udp_src, f);
	f = print(str, "udp_dst", r.udp_dst, f);
	f = print(str, "sctp_src", r.sctp_src, f);
	f = print(str, "sctp_dst", r.sctp_dst, f);
	f = print(str, "icmpv4_type", r.icmpv4_type, f);
	f = print(str, "icmpv4_code", r.icmpv4_code, f);
	f = print(str, "arp_op", r.arp_op, f);
	f = print(str, "arp_spa", r.arp_spa, f);
	f = print(str, "arp_tpa", r.arp_tpa, f);
	f = print(str, "arp_sha", r.arp_sha, f);
	f = print(str, "arp_tha", r.arp_tha, f);
	f = print(str, "ipv6_src", r.ipv6_src, f);
	f = print(str, "ipv6_dst", r.ipv6_dst, f);
	f = print(str, "ipv6_flabel", r.ipv6_flabel, f);
	f = print(str, "icmpv6_type", r.icmpv6_type, f);
	f = print(str, "icmpv6_code", r.icmpv6_code, f);
	f = print(str, "ipv6_nd_target", r.ipv6_nd_target, f);
	f = print(str, "ipv6_nd_sll", r.ipv6_nd_sll, f);
	f = print(str, "ipv6_nd_tll", r.ipv6_nd_tll, f);
	f = print(str, "mpls_label", r.mpls_label, f);
	f = print(str, "mpls_tc", r.mpls_tc, f);
	f = print(str, "mpls_bos", r.mpls_bos, f);
	f = print(str, "pbb_isid", r.pbb_isid, f);
	f = print(str, "tunnel_id", r.tunnel_id, f);
	f = print(str, "ipv6_exthdr", r.ipv6_exthdr, f);
	f = print(str, "pbb_uca", r.pbb_uca, f);
	f = print(str, "tcp_flags", r.tcp_flags, f);
	f = print(str, "actset_output", r.actset_output, f);
	f = print(str, "packet_type", r.packet_type, f);
	return str;
}

Rule_OF_1_5_1::operator std::string() const {
	stringstream ss;
	ss << *this;
	return ss.str();
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
	std::vector<std::string> elems;
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
		vector<string> tokens { istream_iterator<string> { iss },
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
	vector<string> split_slash = split(token, '/');
	vector<string> split_ip = split(split_slash[0], '.');
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
	vector<string> split_slash = split(last_token, '/');

	if (split_slash[1] != "0xFF") {
		Protocol.low = 0;
		Protocol.high = std::numeric_limits<uint16_t>::max();
	} else {
		Protocol.low = Protocol.high = std::stoul(split_slash[0], nullptr, 16);
	}
}

void RuleReader::parse(vector<string>& tokens, vector<iParsedRule*>& ruleset,
		unsigned int cost) {
	// 5 fields: sip, dip, sport, dport, proto = 0 (with@), 1, 2 : 4, 5 : 7, 8

	/*allocate a few more bytes just to be on the safe side to avoid overflow etc*/
	auto temp_rule = new Rule_Ipv4_ACL;
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
		vector<string> tokens { istream_iterator<string> { iss },
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
		throw ifstream::failure("Couldnt open filter set file");
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
	vector<string> split_colon = split(text, ':');
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
		throw ifstream::failure(
				string("Couldnt open filter set file ") + filename);
	}
	string content;
	getline(input_file, content);
	getline(input_file, content);
	vector<string> split_comma = split(content, ',');
	dim = split_comma.size();

	int priority = 0;
	getline(input_file, content);
	vector<string> parts = split(content, ',');
	vector<Range1d<uint32_t>> bounds(parts.size());
	for (size_t i = 0; i < parts.size(); i++) {
		parse_range(bounds[i], parts[i]);
	}

	while (getline(input_file, content)) {
		// 5 fields: sip, dip, sport, dport, proto = 0 (with@), 1, 2 : 4, 5 : 7, 8
		auto temp_rule = new Rule_Ipv4_ACL;
		vector<string> split_comma = split(content, ',');
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
	std::string of_header = "NXST_FLOW reply";
	vector<iParsedRule*> res;
	ifstream in(filename);
	if (!in.is_open()) {
		throw ifstream::failure(
				string("Couldnt open filter set file \"") + filename + "\"");
	}

	string content;
	getline(in, content);
	istringstream iss(content);
	vector<string> tokens { istream_iterator<string> { iss }, istream_iterator<
			string> { } };
	if (content.length() == 0) {
		in.close();
		throw ifstream::failure("File is empty");
	} else if (content[0] == '!') {
		// MSU FORMAT
		vector<string> split_semi = split(tokens.back(), ';');
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
	} else if (content.size() > of_header.size()
			and content.substr(0, of_header.size()) == of_header) {
		auto tmp = parse_openflow(filename);
		res = *reinterpret_cast<vector<iParsedRule*>*>(&tmp);
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

void skip_space_and_comas(const std::string & line, size_t & pos) {
	while (true) {
		char c = line[pos];
		if (c != ' ' and c != ',')
			break;
		pos++;
	}
}

std::string read_id(const std::string & line, size_t & pos) {
	string id;
	while (true) {
		char c = line[pos];
		if (not isalnum(c) and c != '_') {
			break;
		} else {
			id += c;
			pos++;
		}
	}
	return id;
}
size_t read_int(const std::string & line, size_t & pos, int base = 10) {
	string n;
	while (true) {
		char c = line[pos];
		if (not isdigit(c)) {
			break;
		} else {
			n += c;
			pos++;
		}
	}
	return stol(n, 0, base);
}
Range1d<eth_t> read_eth_mac(const std::string & line, size_t & pos) {
	uint64_t val = 0;
	for (int i = 0; i < 6; i++) {
		string n = line.substr(pos, pos + 2);
		val <<= 8;
		val |= stoi(n);
		if (line[pos + 3] == '/') {
			throw runtime_error("not implemented mask of the eth mac");
		}
		pos += 3;
	}
	return {val, val};
}
void read_ipv4(const std::string & line, size_t & pos, Range1d<uint32_t> & ip) {
	int i;
	uint32_t addr = 0;
	for (i = 0; i < 4; i++) {
		addr <<= 8;
		addr |= read_int(line, pos);
		pos++; // skip '.'
	}

	ip = Range1d<uint32_t>(addr, addr);
}
void read_ipv6(const std::string & line, size_t & pos, Range1d<ipv6_t> & ip) {
	throw runtime_error("ipv6 parsing not implemented");
}
void read_ip(const std::string & line, size_t & pos, Range1d<uint32_t> & ipv4,
		Range1d<ipv6_t> & ipv6) {
	for (int i = 0; i < 3; i++) {
		auto c = line[pos + i];
		if (c == ':') {
			read_ipv6(line, pos, ipv6);
		} else if (c == '.') {
			read_ipv4(line, pos, ipv4);
		}
	}
}

Range1d<uint16_t> read_range(const std::string & line, size_t & pos) {
	// int or hex/hex
	uint16_t a = read_int(line, pos, 10);
	if (a == 0 and line[pos] == 'x') {
		pos++;
		a = read_int(line, pos, 16);
		pos += strlen("/0x");
		uint16_t m = read_int(line, pos, 16);
		uint16_t h = a;
		h += ~m;
		return {a, h};
	} else {
		return {a, a};
	}
}

/*
 * read the value of the item in the openflow record
 *
 * search the next <id>= or the end of line then put cursor before the <id> if it was present
 * */
string read_value(string & line, size_t & pos) {
	if (line[pos] != '=')
		return "";
	else {
		size_t start = ++pos;
		while (pos < line.size() and line[pos] != '=') {
			pos++;
		}
		if (pos < line.size()) {
			// there was an <id>= and we need to end before the id token
			pos--;
			while (isalnum(line[pos]) or line[pos] == '_') {
				pos--;
			}
			return line.substr(start, pos - start);
		} else {
			// this is the last key value pair on this lines
			return line.substr(start);
		}
	}
}

std::vector<Rule_OF_1_5_1*> RuleReader::parse_openflow(
		const std::string& filename) {
	vector<Rule_OF_1_5_1*> rules;
	ifstream input_file(filename);
	if (!input_file.is_open()) {
		throw ifstream::failure(
				string("Couldn't open filter set file ") + filename);
	}
	string line;
	getline(input_file, line); // skip the header
	//getline(input_file, line);
	while (getline(input_file, line)) {
		if (line.size() == 0 or line[0] == '#')
			continue;
		auto r = new Rule_OF_1_5_1;
		size_t i = 0;
		while (true) {
			skip_space_and_comas(line, i);
			auto id = read_id(line, i);
			if (id == "")
				break;
			if (id == "in_port") {
				i++; // skip =
				uint32_t d = read_int(line, i);
				r->in_port = {d, d};
			} else if (id == "eth_type") {
				i++; // skip =
				uint16_t d = read_int(line, i);
				r->eth_type = {d, d};
			} else if (id == "dl_src") {
				i++; // skip =
				r->eth_src = read_eth_mac(line, i);
			} else if (id == "dl_dst") {
				i++; // skip =
				r->eth_dst = read_eth_mac(line, i);
			} else if (id == "nw_proto") {
				i++; // skip =
				uint8_t d = read_int(line, i);
				r->ip_proto = {d, d};
			} else if (id == "nw_src") {
				i++; // skip =
				read_ip(line, i, r->ipv4_src, r->ipv6_src);
			} else if (id == "nw_dst") {
				i++; // skip =
				read_ip(line, i, r->ipv4_dst, r->ipv6_dst);
			} else if (id == "tp_dst") {
				i++; // skip =
				r->udp_dst = r->tcp_dst = read_range(line, i);
			} else if (id == "tp_src") {
				i++; // skip =
				r->udp_src = r->tcp_src = read_range(line, i);
			} else if (id == "dl_vlan" or id == "vlan_vid") {
				i++; // skip =
				r->vlan_vid = read_range(line, i);
			} else {
				auto v = read_value(line, i);
				std::cout << __FUNCTION__ << " "<< id << " not implemented";
				std::cout << "\t" <<"=" << v << endl;
			}
		}
		rules.push_back(r);
	}
	return rules;
}

std::vector<std::pair<iParsedRule*, size_t>> parse_ruleset_file(
		const std::string & rule_file) {
	RuleReader rp;
	auto _rules = rp.parse_rules(rule_file);
	std::vector<std::pair<iParsedRule*, size_t>> rules(_rules.size());
	std::unordered_map<iParsedRule*, size_t> prefix_len;
	for (auto _r : _rules) {
		auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r);
		prefix_len[_r] = __r->cummulative_prefix_len();
	}

	auto rule_specifity_sort_predicate =
			[&prefix_len](iParsedRule* a, iParsedRule* b) {
				return prefix_len[a] > prefix_len[b];
			};
	std::sort(_rules.begin(), _rules.end(), rule_specifity_sort_predicate);

	size_t i = 0;
	for (auto _r : _rules) {
		auto __r = reinterpret_cast<Rule_Ipv4_ACL*>(_r);
		rules[i] = {__r, _rules.size() - i};
		i++;
	}

	return rules;
}

}
