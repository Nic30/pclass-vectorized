#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE pcv_test

#include "test_common.h"
#include <random>
//#include <pcv/partiton_sort/mempool.h>
#include <pcv/partiton_sort/mempool_mock.h>

using namespace pcv;
using namespace std;

BOOST_AUTO_TEST_SUITE( pcv_testsuite )

// different classes to make tests independent
class MInt0: public ObjectWithStaticMempool<MInt0, 32, false> {
public:
	int val;
};

class MInt1: public ObjectWithStaticMempool<MInt0, 32, false> {
public:
	int val[300];
};

BOOST_AUTO_TEST_CASE( simple) {
	std::vector<MInt0 *> vals;
	for (size_t i = 0; i < 32; i++) {
		auto v = new MInt0;
		v->val = i;
		vals.push_back(v);
	}
	for (size_t i = 0; i < 32; i++) {
		BOOST_CHECK_EQUAL(vals[i]->val, i);
	}
	BOOST_CHECK_THROW(new MInt0, bad_alloc);
	BOOST_CHECK_THROW(new MInt0, bad_alloc);

	delete vals.back();
	vals.pop_back();

	auto v = new MInt0;
	v->val = 31;
	vals.push_back(v);
	for (size_t i = 0; i < 32; i++) {
		BOOST_CHECK_EQUAL(vals[i]->val, i);
	}

	for (size_t i = 0; i < 32; i++) {
		delete vals[i];
	}

	vals.clear();
	for (size_t i = 0; i < 32; i++) {
		auto v = new MInt0;
		v->val = i + 32;
		vals.push_back(v);
	}
	for (size_t i = 0; i < 32; i++) {
		BOOST_CHECK_EQUAL(vals[i]->val, i + 32);
	}
	for (size_t i = 0; i < 32; i++) {
		delete vals[i];
	}
}

BOOST_AUTO_TEST_CASE( randomized ) {

	auto rand = std::bind(std::uniform_int_distribution<int>(0, 1), mt19937(0));

	std::vector<MInt1*> vals;
	for (size_t i = 0; i < 50000; i++) {
		if ((rand() and vals.size() < 32) or vals.size() == 0) {
			vals.push_back(new MInt1);
		} else {
			delete vals.back();
			vals.pop_back();
		}
	}

}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
