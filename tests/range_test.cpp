#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"

#include <limits>
#include <pcv/common/range.h>

using namespace std;
using namespace pcv;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )
BOOST_AUTO_TEST_CASE( masks ) {
	{
		Range1d<uint32_t> a;
		a.low = 0xff000000;
		a.high = 0xffffffff;
		auto m = a._get_mask_le();
		BOOST_CHECK_EQUAL(m.first, 0xff000000);
		BOOST_CHECK_EQUAL(m.second, 8);

		auto b = a.to_be();
		BOOST_CHECK_EQUAL(b.low, 0x000000ff);
	}
	{
		Range1d<uint32_t> a;
		a.low = 0xffff0000;
		a.high = 0xffffffff;
		auto m = a._get_mask_le();
		BOOST_CHECK_EQUAL(m.first, 0xffff0000);
		BOOST_CHECK_EQUAL(m.second, 16);

	}
	{
		Range1d<uint32_t> a;
		a.low = 0x0000ffff;
		a.high = 0xffffffff;
		auto m = a._get_mask_be();
		BOOST_CHECK_EQUAL(m.first, 0x0000ffff);
		BOOST_CHECK_EQUAL(m.second, 16);
	}
	{
		Range1d<uint16_t> a;
		a.low = 0xfff0;
		a.high = 0xffff;
		auto m = a._get_mask_be();
		BOOST_CHECK_EQUAL(m.first, 0xf0);
		BOOST_CHECK_EQUAL(m.second, 4);
		m = a._get_mask_le();
		BOOST_CHECK_EQUAL(m.first, 0xfff0);
		BOOST_CHECK_EQUAL(m.second, 12);
	}

	{
		auto a = Range1d<uint16_t>::from_mask((199 << 8) | 190, 0xFFFF);
		auto b = Range1d<uint16_t>::from_mask((198 << 8), 0xFE00);
		BOOST_CHECK(a.overlaps(a));
		BOOST_CHECK(a.overlaps(b));
		BOOST_CHECK(b.overlaps(a));
		BOOST_CHECK(b.overlaps(b));
	}

}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
