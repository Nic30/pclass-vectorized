#pragma once

#include <functional>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <boost/test/unit_test.hpp>

#define BOOST_CHECK_EQUAL_MESSAGE(L, R, M)      { BOOST_TEST_INFO(M); BOOST_CHECK_EQUAL(L, R); }
