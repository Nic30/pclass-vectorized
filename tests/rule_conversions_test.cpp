#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include <pcv/rule_parser/rule.h>
#include "test_common.h"


using namespace pcv;
using namespace pcv::rule_conv_fn;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )

BOOST_AUTO_TEST_CASE( simple) {
	Rule_Ipv4_ACL r;
	r.sip = {0x01020304, 0x05060708};
	r.dip = {0x090a0b0c, 0x0d0e0f10};
	r.sport = {0x1112, 0x1314};
	r.dport = {0x1516, 0x1718};
	r.proto = {0x19, 0x1a};

	auto ra = rule_to_array_16b(r);
	auto r2 = rule_from_array(ra);
	BOOST_CHECK(r.sip == r2.sip);
	BOOST_CHECK(r.dip == r2.dip);
	BOOST_CHECK(r.sport == r2.sport);
	BOOST_CHECK(r.dport == r2.dport);
	BOOST_CHECK(r.proto == r2.proto);
}


BOOST_AUTO_TEST_SUITE_END()
