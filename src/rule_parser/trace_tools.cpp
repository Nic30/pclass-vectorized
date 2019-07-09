// original author: David E. Taylor
// Applied Research Laboratory
// Department of Computer Science and Engineering
// Washington University in Saint Louis
// det3@arl.wustl.edu
//
// Functions for generating synthetic trace of headers
//

#include <random>
#include <pcv/rule_parser/trace_tools.h>
#include <pcv/rule_parser/rule.h>
#include <pcv/common/range.h>
#include <byteswap.h>
#include <utility>

namespace pcv {

void ramdom_corner_push_value(packet_t & packet, size_t & byte_offset,
		const Range1d<uint32_t> & val, bool use_low, bool big_endian) {
	assert(byte_offset % 2 == 0);
	uint32_t v = use_low ? val.low : val.high;
	uint16_t p[2];
	if (big_endian) {
		v = __bswap_32(v);
		p[0] = v;
		p[1] = v >> 16;
	} else {
		p[0] = v;
		p[1] = v >> 16;
	}

	packet[byte_offset / 2] = p[0];
	packet[byte_offset / 2 + 1] = p[1];
	byte_offset += sizeof(uint32_t);
}

void ramdom_corner_push_value(packet_t & packet, size_t & byte_offset,
		const Range1d<uint16_t> & val, bool use_low, bool big_endian) {
	uint16_t v = use_low ? val.low : val.high;
	if (big_endian) {
		v = __bswap_16(v);
	}
	packet[byte_offset / 2] = v;
	byte_offset += sizeof(uint16_t);
}

void random_corner(const Rule_Ipv4_ACL & rule, packet_t & new_hdr, int d,
		std::mt19937 & rand, bool big_endian) {
	std::uniform_int_distribution<> rand_bit(0, 1);
	size_t byte_offset = 0;
	double p = rand_bit(rand);
	ramdom_corner_push_value(new_hdr, byte_offset, rule.sip, p < 0.5,
			big_endian);
	p = rand_bit(rand);
	ramdom_corner_push_value(new_hdr, byte_offset, rule.dip, p < 0.5,
			big_endian);
	p = rand_bit(rand);
	ramdom_corner_push_value(new_hdr, byte_offset, rule.sport, p < 0.5,
			big_endian);
	p = rand_bit(rand);
	ramdom_corner_push_value(new_hdr, byte_offset, rule.dport, p < 0.5,
			big_endian);
	p = rand_bit(rand);
	ramdom_corner_push_value(new_hdr, byte_offset, rule.proto, p < 0.5,
			big_endian);
}

size_t pareto_distrib(float a, float b, std::mt19937 & rand) {
	if (b == 0)
		return 1;

	std::uniform_int_distribution<> rand_bit(0, 1);
	// Select random number
	double p = rand_bit(rand);

	double x = (double) b / pow((double) (1 - p), (double) (1 / (double) a));
	return ceil(x);
}

// Generate headers
// a,b in ClassBench are 1 0.1
// generate at least 'threshold' number of packets
std::vector<packet_t> header_gen(std::vector<const Rule_Ipv4_ACL *>& filters,
		float a, float b, int threshold, std::mt19937 & rand, bool big_endian) {
	constexpr size_t d = 7;
	int num_headers = 0;
	int fsize = filters.size();

	std::vector<packet_t> temp_packets;
	std::uniform_int_distribution<> RandFilt_dis(0, fsize - 1);
	// Generate headers
	while (num_headers < threshold) {
		// Pick a random filter

		packet_t new_hdr;
		// Pick a random corner of the filter for a header
		auto fi = RandFilt_dis(rand);
		random_corner(*filters[fi], new_hdr, d, rand, big_endian);

		// Select number of copies to add to header list
		// from Pareto distribution
		size_t copies_n = pareto_distrib(a, b, rand);

		// Add to header list
		for (size_t i = 0; i < copies_n; i++) {
			temp_packets.push_back(new_hdr);
		}
		// Increment number of headers
		num_headers += copies_n;
	}

	return std::vector<packet_t>(begin(temp_packets),
			begin(temp_packets) + threshold);
}

std::vector<packet_t> generate_packets_from_ruleset(
		std::vector<const Rule_Ipv4_ACL *>& filters, size_t num_packets,
		size_t seed, bool big_endian) {
	if (filters.empty())
		throw std::runtime_error(
				"can not generate packets from empty rule set");

	std::mt19937 rand(seed);
	return header_gen(filters, 1, 0.1f, num_packets, rand, big_endian);
}
}
