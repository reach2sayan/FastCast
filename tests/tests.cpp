//
// Created by sayan on 10/4/25.
//

// fastcast_test.cpp
#define CATCH_CONFIG_MAIN
#include "../fastcast.hpp"
#include "catch.hpp"
#include "utilities.hpp"

TEST_CASE("Version", "[fastcast]") {
  CHECK(FASTCAST_VERSION_MAJOR == 1);
  CHECK(FASTCAST_VERSION_MINOR == 0);
  CHECK(FASTCAST_VERSION_PATCH == 0);
  CHECK(FASTCAST_VERSION == 10000);
  CHECK(std::string(FASTCAST_VERSION_STRING) == "1.0.0");
}

TEST_CASE("SimpleHierarchy_DynamicVsFast", "[fastcast]") {
  SimpleB b;
  SimpleA &a = b;
  SimpleB &db = dynamic_cast<SimpleB &>(a);
  CHECK(db.method_b_only() == 42);
  SimpleB &fb = fast_cast<SimpleB &>(a);
  CHECK(fb.method_b_only() == 42);
}

TEST_CASE("SimpleHierarchy_PointerNullCheck", "[fastcast]") {
  SimpleA *ap = nullptr;
  CHECK(dynamic_cast<SimpleB *>(ap) == nullptr);
  CHECK(fast_cast<SimpleB *>(ap) == nullptr);
}

TEST_CASE("ComplexHierarchy_DynamicVsFast", "[fastcast]") {
  ComplexG g;
  ComplexA &a = g;
  ComplexG &dg = dynamic_cast<ComplexG &>(a);
  ComplexG &fg = fast_cast<ComplexG &>(a);
  CHECK(dg.method_g_only() == 1729);
  CHECK(fg.method_g_only() == 1729);
}

TEST_CASE("ComplexHierarchy_PointerNullCheck", "[fastcast]") {
  ComplexA *ap = nullptr;
  CHECK(dynamic_cast<ComplexG *>(ap) == nullptr);
  CHECK(fast_cast<ComplexG *>(ap) == nullptr);
}

TEST_CASE("ComplexHierarchy_CrossCast", "[fastcast]") {
  ComplexG g;
  ComplexA &a = g;
  // Cross-cast down to ComplexF through ComplexA
  ComplexF *df = dynamic_cast<ComplexF *>(&a);
  ComplexF *ff = fast_cast<ComplexF *>(&a);

  REQUIRE(df != nullptr);
  REQUIRE(ff != nullptr);
  CHECK(df->method() == ff->method());
}

TEST_CASE("Simple_Success_Ptr", "[fastcast]") {
  SimpleB b;
  SimpleA *a = &b;

  auto *bp = dynamic_cast<SimpleB *>(a);
  auto *fp = fast_cast<SimpleB *>(a);

  REQUIRE(bp != nullptr);
  REQUIRE(fp != nullptr);
  CHECK(bp->method_b_only() == 42);
  CHECK(fp->method_b_only() == 42);
}

TEST_CASE("Simple_Failure_PtrNull", "[fastcast]") {
  SimpleA *a = new SimpleA{};
  CHECK(dynamic_cast<SimpleB *>(a) == nullptr);
  CHECK(fast_cast<SimpleB *>(a) == nullptr);
  delete a;
}

TEST_CASE("Simple_Failure_BadCastRef", "[fastcast]") {
  SimpleA a;
  CHECK_THROWS_AS((void)dynamic_cast<SimpleB &>(a), std::bad_cast);
  CHECK_THROWS_AS((void)fast_cast<SimpleB &>(a), std::bad_cast);
}

TEST_CASE("Simple_ConstCorrectness", "[fastcast]") {
  const SimpleB b;
  const SimpleA &a = b;
  auto &br = dynamic_cast<const SimpleB &>(a);
  auto &fr = fast_cast<const SimpleB &>(a);
  CHECK(br.method_b_only() == fr.method_b_only());
}

TEST_CASE("Complex_Success_Ref", "[fastcast]") {
  ComplexE w;
  ComplexA &x = w;

  auto &dr = dynamic_cast<ComplexE &>(x);
  auto &fr = fast_cast<ComplexE &>(x);

  CHECK(dr.method() == 2520);
  CHECK(fr.method() == 2520);
}

TEST_CASE("Complex_Success_CrossCast", "[fastcast]") {
  ComplexE w;
  ComplexA *x = &w;

  auto *dz = dynamic_cast<ComplexB *>(x);
  auto *fz = fast_cast<ComplexB *>(x);

  REQUIRE(dz != nullptr);
  REQUIRE(fz != nullptr);
  CHECK(dz->method() == fz->method());
}

TEST_CASE("Complex_Failure_NullPtr", "[fastcast]") {
  ComplexA *x = nullptr;
  CHECK(dynamic_cast<ComplexF *>(x) == nullptr);
  CHECK(fast_cast<ComplexF *>(x) == nullptr);
}

TEST_CASE("Complex_Failure_Unrelated", "[fastcast]") {
  ComplexE w;
  ComplexB &z1 = w;
  CHECK_THROWS_AS((void)dynamic_cast<ComplexG &>(
                      reinterpret_cast<ComplexA &>(z1)), // unrelated
                  std::bad_cast);
  CHECK_THROWS_AS(
      (void)fast_cast<ComplexF &>(reinterpret_cast<ComplexA &>(z1)),
      std::bad_cast);
}

TEST_CASE("SameType", "[fastcast]") {
  Derived d;
  Derived *dp = &d;
  auto r1 = fastcast::fast_cast<Derived *>(dp);
  CHECK(r1 == dp);
}

TEST_CASE("DerivedToBase_StaticCastPath", "[fastcast]") {
  Derived d;
  Derived *dp = &d;
  Base *bp = fastcast::fast_cast<Base *>(dp);
  CHECK(bp == static_cast<Base *>(dp));
}

TEST_CASE("BaseToDerived_RuntimePath", "[fastcast]") {
  Derived d;
  Base *bp = &d;
  auto dp = fastcast::fast_cast<Derived *>(bp);
  CHECK(dp == dynamic_cast<Derived *>(bp));
}

TEST_CASE("MultipleInheritance_Failure", "[fastcast]") {
  Multi m;
  Base *bp = &m;
  auto ap = fastcast::fast_cast<AnotherBase *>(bp);
  CHECK(ap == dynamic_cast<AnotherBase *>(bp));
}

TEST_CASE("FailureCaching", "[fastcast]") {
  Base b;
  Base *bp = &b;
  auto dp = fastcast::fast_cast<Derived *>(bp);
  CHECK(dp == nullptr);

  // call again, should hit failure cache fast
  auto dp2 = fastcast::fast_cast<Derived *>(bp);
  CHECK(dp2 == nullptr);
}
