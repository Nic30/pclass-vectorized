// original author: David E. Taylor
// Applied Research Laboratory
// Department of Computer Science and Engineering
// Washington University in Saint Louis
// det3@arl.wustl.edu
//
// Functions for generating synthetic trace of headers
//

#include <pcv/rule_parser/trace_tools.h>
#include <random>

namespace pcv {

void RandomCorner(const std::array<Range1d<uint16_t>, 7> & rule,
		packet_t & new_hdr, int d, std::mt19937 & rand) {

	// Random number
	double p;
	std::uniform_int_distribution<> rand_bit(0, 1);

	for (int i = 0; i < d; i++) {
		p = rand_bit(rand);
		// Select random number
		if (p < 0.5) {
			// Choose low extreme of field
			new_hdr[i] = rule[i].low;
		} else {
			// Choose high extreme of field
			new_hdr[i] = rule[i].high;
		}
	}
	return;
}

int pareto_distrib(float a, float b, std::mt19937 & rand) {
	if (b == 0)
		return 1;
	std::uniform_int_distribution<> rand_bit(0, 1);

	// Random number
	double p;
	// Select random number
	p = rand_bit(rand);

	double x = (double) b / pow((double) (1 - p), (double) (1 / (double) a));
	int Num = (int) ceil(x);
	return Num;
}

// Generate headers
// a,b in ClassBench are 1 0.1 
// generate at least 'threshold' number of packets
// To ensure the generated dataset is deterministic, call this first!!
std::vector<packet_t> header_gen(std::vector<const Rule_Ipv4 *>& filters,
		float a, float b, int threshold, std::mt19937 & rand) {
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
		auto f_as_array = rule_conv_fn::rule_to_array<uint16_t, 7>(
				*filters[fi]);
		RandomCorner(f_as_array, new_hdr, d, rand);

		// Select number of copies to add to header list
		// from Pareto distribution
		int Copies = pareto_distrib(a, b, rand);

		// Add to header list
		for (int i = 0; i < Copies; i++) {
			temp_packets.push_back(new_hdr);
		}
		// Increment number of headers
		num_headers += Copies;
	}

	return std::vector<packet_t>(begin(temp_packets),
			begin(temp_packets) + threshold);
}

std::vector<packet_t> generate_packets_from_ruleset(
		std::vector<const Rule_Ipv4 *>& filters, int num_packets, int seed) {
	if (filters.empty())
		throw std::runtime_error(
				"can not generate packets from empty rule set");

	std::mt19937 rand(seed);
	return header_gen(filters, 1, 0.1f, num_packets, rand);
}
}
