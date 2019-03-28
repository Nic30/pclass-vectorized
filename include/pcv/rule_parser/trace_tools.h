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

namespace pcv {

using packet_t = std::array<uint16_t, 7>;

std::vector<packet_t> generate_packets_from_ruleset(
		std::vector<const Rule_Ipv4*>& filters, int num_packets, int seed = 0);

}
