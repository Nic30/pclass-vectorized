// origina author: David E. Taylor
// Applied Research Laboratory
// Department of Computer Science and Engineering
// Washington University in Saint Louis
// det3@arl.wustl.edu
//
// Functions for generating synthetic trace of headers
//
#pragma once

#include <vector>
#include <pcv/rule_parser/rule.h>
#include <random>

namespace pcv {

using packet_t = std::array<uint16_t, 7>;

void random_corner(const Rule_Ipv4_ACL & rule, packet_t & new_hdr, int d,
		std::mt19937 & rand, bool big_endian);

std::vector<packet_t> generate_packets_from_ruleset(
		std::vector<const Rule_Ipv4_ACL*>& filters, size_t num_packets,
		size_t seed = 0, bool big_endian = false);

}
