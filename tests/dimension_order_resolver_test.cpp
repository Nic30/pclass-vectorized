#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"
#include <vector>
#include <array>

#include <pcv/partition_sort/dimension_order_resolver.h>
#include <pcv/partition_sort/b_tree.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )
static constexpr size_t D = 4;
using R1d = Range1d<unsigned>;
R1d any(0, numeric_limits<unsigned>::max());
using rule_range_part_t = std::array<R1d, D>;
using rule_spec_t = std::pair<rule_range_part_t, unsigned>;
using Resolver = GreedyDimensionOrderResolver<rule_spec_t, D, unsigned>;

BOOST_AUTO_TEST_CASE( simple_non_overlapping ) {
	vector<rule_spec_t> rules;
	for (int i = 0; i < 5; i++) {
		rule_spec_t r = { { any, R1d(0, 0), R1d(i, i), any }, 0 };
		rules.push_back(r);
	}
	array<unsigned, D> order;
	for (unsigned i = 0; i < D; i++) {
		order[i] = D - i - 1;
	}

	Resolver r(rules, order);
	auto res = r.resolve();
	BOOST_TEST(res.first == vector<unsigned>({1,2,3,0}),
			boost::test_tools::per_element());
	BOOST_TEST(res.second == 2);
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
