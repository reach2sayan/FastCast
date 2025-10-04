//
// Created by sayan on 10/4/25.
//

// fastcast_test.cpp
#define BOOST_TEST_MODULE FastCastTests
#include "fastcast.hpp"
#include "utilities.hpp"
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(SimpleHierarchy_DynamicVsFast) {
  SimpleB b;
  SimpleA &a = b;
  SimpleB &db = dynamic_cast<SimpleB &>(a);
  BOOST_CHECK_EQUAL(db.method_b_only(), 42);
  SimpleB &fb = fast_dynamic_cast<SimpleB &>(a);
  BOOST_CHECK_EQUAL(fb.method_b_only(), 42);
}

BOOST_AUTO_TEST_CASE(SimpleHierarchy_PointerNullCheck) {
  SimpleA *ap = nullptr;
  BOOST_CHECK(dynamic_cast<SimpleB *>(ap) == nullptr);
  BOOST_CHECK(fast_dynamic_cast<SimpleB *>(ap) == nullptr);
}

BOOST_AUTO_TEST_CASE(ComplexHierarchy_DynamicVsFast) {
  ComplexG g;
  ComplexA &a = g;
  ComplexG &dg = dynamic_cast<ComplexG &>(a);
  ComplexG &fg = fast_dynamic_cast<ComplexG &>(a);
  BOOST_CHECK_EQUAL(dg.method_g_only(), 1729);
  BOOST_CHECK_EQUAL(fg.method_g_only(), 1729);
}

BOOST_AUTO_TEST_CASE(ComplexHierarchy_PointerNullCheck) {
  ComplexA *ap = nullptr;
  BOOST_CHECK(dynamic_cast<ComplexG *>(ap) == nullptr);
  BOOST_CHECK(fast_dynamic_cast<ComplexG *>(ap) == nullptr);
}

BOOST_AUTO_TEST_CASE(ComplexHierarchy_CrossCast) {
  ComplexG g;
  ComplexA &a = g;
  // Cross-cast down to ComplexF through ComplexA
  ComplexF *df = dynamic_cast<ComplexF *>(&a);
  ComplexF *ff = fast_dynamic_cast<ComplexF *>(&a);

  BOOST_REQUIRE(df != nullptr);
  BOOST_REQUIRE(ff != nullptr);
  BOOST_CHECK_EQUAL(df->method(), ff->method());
}
