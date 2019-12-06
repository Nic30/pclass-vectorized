#include <random>

#include <pcv/rule_parser/classbench_rule_parser.h>
#include <pcv/common/range.h>

using namespace std;
using namespace pcv;

/*
 * Program which generates random exact match rules in classbench-ng format
 * */
int main(int argc, const char * argv[]) {
	assert(argc == 1 + 2);
	size_t seed = atoll(argv[1]);
	size_t rule_cnt = atoll(argv[2]);
	{
		std::mt19937 rand (seed);
		std::uniform_int_distribution<uint32_t> rand_ipv4(0, std::numeric_limits<uint32_t>::max());
		std::uniform_int_distribution<uint16_t> rand_port(0, std::numeric_limits<uint16_t>::max());
		std::uniform_int_distribution<uint16_t> rand_proto(0, std::numeric_limits<uint8_t>::max());

		// load rules in to a classifier tree
		for (size_t i = 0; i < rule_cnt; i++) {
			Rule_Ipv4_ACL r;
			r.sip.low = r.sip.high = rand_ipv4(rand);
			r.dip.low = r.dip.high = rand_ipv4(rand);
			r.sport.low = r.sport.high = rand_port(rand);
			r.dport.low = r.dport.high = rand_port(rand);
			r.proto.low = r.proto.high = rand_proto(rand);
			cout << r << endl;
		}
	}
	return 0;
}
