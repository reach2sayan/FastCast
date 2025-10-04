//
// Created by sayan on 10/4/25.
//

// fastcast_test.cpp
#define BOOST_TEST_MODULE FastCastTests
#include "fastcast.hpp"
#include "utilities.hpp"
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(FastCastChecks)

BOOST_AUTO_TEST_CASE(SimpleHierarchy_DynamicVsFast) {
  SimpleB b;
  SimpleA &a = b;
  SimpleB &db = dynamic_cast<SimpleB &>(a);
  BOOST_CHECK_EQUAL(db.method_b_only(), 42);
  SimpleB &fb = fast_cast<SimpleB &>(a);
  BOOST_CHECK_EQUAL(fb.method_b_only(), 42);
}

BOOST_AUTO_TEST_CASE(SimpleHierarchy_PointerNullCheck) {
  SimpleA *ap = nullptr;
  BOOST_CHECK(dynamic_cast<SimpleB *>(ap) == nullptr);
  BOOST_CHECK(fast_cast<SimpleB *>(ap) == nullptr);
}

BOOST_AUTO_TEST_CASE(ComplexHierarchy_DynamicVsFast) {
  ComplexG g;
  ComplexA &a = g;
  ComplexG &dg = dynamic_cast<ComplexG &>(a);
  ComplexG &fg = fast_cast<ComplexG &>(a);
  BOOST_CHECK_EQUAL(dg.method_g_only(), 1729);
  BOOST_CHECK_EQUAL(fg.method_g_only(), 1729);
}

BOOST_AUTO_TEST_CASE(ComplexHierarchy_PointerNullCheck) {
  ComplexA *ap = nullptr;
  BOOST_CHECK(dynamic_cast<ComplexG *>(ap) == nullptr);
  BOOST_CHECK(fast_cast<ComplexG *>(ap) == nullptr);
}

BOOST_AUTO_TEST_CASE(ComplexHierarchy_CrossCast) {
  ComplexG g;
  ComplexA &a = g;
  // Cross-cast down to ComplexF through ComplexA
  ComplexF *df = dynamic_cast<ComplexF *>(&a);
  ComplexF *ff = fast_cast<ComplexF *>(&a);

  BOOST_REQUIRE(df != nullptr);
  BOOST_REQUIRE(ff != nullptr);
  BOOST_CHECK_EQUAL(df->method(), ff->method());
}

BOOST_AUTO_TEST_CASE(Simple_Success_Ptr) {
  SimpleB b;
  SimpleA *a = &b;

  auto *bp = dynamic_cast<SimpleB *>(a);
  auto *fp = fast_cast<SimpleB *>(a);

  BOOST_REQUIRE(bp != nullptr);
  BOOST_REQUIRE(fp != nullptr);
  BOOST_CHECK_EQUAL(bp->method_b_only(), 42);
  BOOST_CHECK_EQUAL(fp->method_b_only(), 42);
}

BOOST_AUTO_TEST_CASE(Simple_Failure_PtrNull) {
  SimpleA *a = new SimpleA{};
  BOOST_CHECK(dynamic_cast<SimpleB *>(a) == nullptr);
  BOOST_CHECK(fast_cast<SimpleB *>(a) == nullptr);
  delete a;
}

BOOST_AUTO_TEST_CASE(Simple_Failure_BadCastRef) {
  SimpleA a;
  try {
    (void)dynamic_cast<SimpleB &>(a);
    BOOST_FAIL("Expected std::bad_cast");
  } catch (const std::bad_cast &) {
    BOOST_CHECK(true);
  }
  try {
    (void)fast_cast<SimpleB &>(a);
    BOOST_FAIL("Expected std::bad_cast");
  } catch (const std::bad_cast &) {
    BOOST_CHECK(true);
  }
}

BOOST_AUTO_TEST_CASE(Simple_ConstCorrectness) {
  const SimpleB b;
  const SimpleA &a = b;
  auto &br = dynamic_cast<const SimpleB &>(a);
  auto &fr = fast_cast<const SimpleB &>(a);
  BOOST_CHECK_EQUAL(br.method_b_only(), fr.method_b_only());
}

BOOST_AUTO_TEST_CASE(Complex_Success_Ref) {
  ComplexE w;
  ComplexA &x = w;

  auto &dr = dynamic_cast<ComplexE &>(x);
  auto &fr = fast_cast<ComplexE &>(x);

  BOOST_CHECK_EQUAL(dr.method(), 2520);
  BOOST_CHECK_EQUAL(fr.method(), 2520);
}

BOOST_AUTO_TEST_CASE(Complex_Success_CrossCast) {
  ComplexE w;
  ComplexA *x = &w;

  auto *dz = dynamic_cast<ComplexB *>(x);
  auto *fz = fast_cast<ComplexB *>(x);

  BOOST_REQUIRE(dz != nullptr);
  BOOST_REQUIRE(fz != nullptr);
  BOOST_CHECK_EQUAL(dz->method(), fz->method());
}

BOOST_AUTO_TEST_CASE(Complex_Failure_NullPtr) {
  ComplexA *x = nullptr;
  BOOST_CHECK(dynamic_cast<ComplexF *>(x) == nullptr);
  BOOST_CHECK(fast_cast<ComplexF *>(x) == nullptr);
}

BOOST_AUTO_TEST_CASE(Complex_Failure_Unrelated) {
  ComplexE w;
  ComplexB &z1 = w;
  try {
    (void)dynamic_cast<ComplexG &>(
        reinterpret_cast<ComplexA &>(z1)); // unrelated
    BOOST_FAIL("Expected std::bad_cast");
  } catch (const std::bad_cast &) {
    BOOST_CHECK(true);
  }
  try {
    (void)fast_cast<ComplexF &>(reinterpret_cast<ComplexA &>(z1));
    BOOST_FAIL("Expected std::bad_cast");
  } catch (const std::bad_cast &) {
    BOOST_CHECK(true);
  }
}

BOOST_AUTO_TEST_CASE(SameType) {
  Derived d;
  Derived *dp = &d;
  auto r1 = fastcast::fast_cast<Derived *>(dp);
  BOOST_CHECK(r1 == dp);
}

BOOST_AUTO_TEST_CASE(DerivedToBase_StaticCastPath) {
  Derived d;
  Derived *dp = &d;
  Base *bp = fastcast::fast_cast<Base *>(dp);
  BOOST_CHECK(bp == static_cast<Base *>(dp));
}

BOOST_AUTO_TEST_CASE(BaseToDerived_RuntimePath) {
  Derived d;
  Base *bp = &d;
  auto dp = fastcast::fast_cast<Derived *>(bp);
  BOOST_CHECK(dp == dynamic_cast<Derived *>(bp));
}

BOOST_AUTO_TEST_CASE(MultipleInheritance_Failure) {
  Multi m;
  Base *bp = &m;
  auto ap = fastcast::fast_cast<AnotherBase *>(bp);
  BOOST_CHECK(ap == dynamic_cast<AnotherBase *>(bp));
}

BOOST_AUTO_TEST_CASE(FailureCaching) {
  Base b;
  Base *bp = &b;
  auto dp = fastcast::fast_cast<Derived *>(bp);
  BOOST_CHECK(dp == nullptr);

  // call again, should hit failure cache fast
  auto dp2 = fastcast::fast_cast<Derived *>(bp);
  BOOST_CHECK(dp2 == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
