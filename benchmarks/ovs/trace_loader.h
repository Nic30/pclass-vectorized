#pragma once
#include <vector>
#include <map>
#include <iostream>

#include <pcv/rule_parser/rule.h>
#include <pcv/rule_parser/trace_tools.h>
#include <boost/lexical_cast.hpp>

#include "lib/pvector.h"
#include "lib/classifier.h"
#undef atomic_store
#undef atomic_compare_exchange_strong_explicit
#undef atomic_compare_exchange_strong
#undef atomic_compare_exchange_weak_explicit
#undef atomic_compare_exchange_weak

namespace pcv {
namespace ovs {

class OvsTraceLoder {
	std::string current;

	OvsTraceLoder();

	/*
	 * Consume id and optionally ' ' or ',' from input
	 * */
	std::string get_id();
	std::string get_hex_digit();
	std::string get_dec_digit();
	void parse_eth_mac(struct eth_addr &res);
	void parse_in_port(union flow_in_port &res);
	void parse_ipv4(ovs_be32 &res);
	void skip_ws_and_comma();
	void parse_ipv6(struct in6_addr &res);
	bool startswith(const std::string &main_str, const std::string &to_match);
	struct flow parse_trace_record(const std::string &s);
	std::vector<struct flow> load_from_file(const std::string &file_name);
};

}
}
