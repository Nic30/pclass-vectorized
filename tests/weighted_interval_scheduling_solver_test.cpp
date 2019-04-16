#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"
#include <vector>

#include <pcv/partition_sort/weighted_interval_scheduling_solver.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )

BOOST_AUTO_TEST_CASE( simple_non_overlapping ) {
	unsigned N = 10;
	vector<Range1d<unsigned>> vals;
	for (unsigned i = 0; i < N; i++)
		vals.push_back( { i, i });

	WeightedIntervalSchedulingSolver<unsigned> schd(vals);

	auto res = schd.findMaxWeightIntervalSequence();
	BOOST_CHECK_EQUAL(res, N);

	for (unsigned i = 0; i < N - 1; i++)
		BOOST_CHECK_EQUAL(schd.table[i], i + 1);
}

BOOST_AUTO_TEST_CASE( simple_2_mixed_sequences ) {
	unsigned N = 2;
	vector<Range1d<unsigned>> vals;
	for (unsigned i = 0; i < N; i++)
		vals.push_back( { i * 10, (i + 1) * 10 });
	for (unsigned i = 0; i < 2 * N; i++)
		vals.push_back( { i * 5, (i + 1) * 5 });

	WeightedIntervalSchedulingSolver<unsigned> schd(vals);

	auto res = schd.findMaxWeightIntervalSequence();
	BOOST_CHECK_EQUAL(res, 2 * N);
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
