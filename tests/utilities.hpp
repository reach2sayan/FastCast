//
// Created by sayan on 10/4/25.
//
// Class hierarchies shared by the tests and the benchmarks. Everything here
// is C++11 so the same file serves every standard the library supports.

#ifndef FASTCAST_UTILITIES_HPP
#define FASTCAST_UTILITIES_HPP

//
// Simple hierarchy:
//   A
//   |
//   B
//
struct SimpleA {
  virtual ~SimpleA() = default;
};
struct SimpleB : public SimpleA {
  virtual int method_b_only() const { return 42; }
};

/*
 Complex hierarchy with virtual and multiple inheritance:

     A
     |
     B          (virtual base of C and E)
     | \
     C  E
     |  |
     D  F
      \/
       G
*/

struct ComplexA {
  virtual ~ComplexA() = default;
  virtual int method() { return 1; }
};
struct ComplexB : public ComplexA {
  int method() override { return 2; }
};
struct ComplexC : public virtual ComplexB {
  int method() override { return 3; }
};
struct ComplexD : public ComplexC {
  int method() override { return 4; }
};
struct ComplexE : public virtual ComplexB {
  int method() override { return 2520; }
};
struct ComplexF : public ComplexE {
  int method() override { return 6; }
};
struct ComplexG : public ComplexD, public ComplexF {
  int method() override { return 1729; }
  virtual int method_g_only() { return method(); }
};

//
// Multiple (non-virtual) inheritance from two unrelated polymorphic bases.
// The AnotherBase subobject of Multi lives at a non-zero offset, so
// Base* <-> AnotherBase* cross-casts need a real pointer adjustment.
//
struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};
struct AnotherBase {
  virtual ~AnotherBase() = default;
};
struct Multi : Base, AnotherBase {};

//
// Two distinct Base subobjects inside a single object. Each has its own
// vtable pointer, so the cache must tell them apart.
//
struct LeftBase : Base {
  int left = 1;
};
struct RightBase : Base {
  int right = 2;
};
struct TwoBases : LeftBase, RightBase {};

//
// A family of distinct leaf types for cache-churn tests.
//
template <int N> struct Leaf : Base {
  int id() const { return N; }
};

//
// Non-polymorphic types (only static/identity casts are legal on these).
//
struct PlainBase {
  int x = 0;
};
struct PlainDerived : PlainBase {};

#endif // FASTCAST_UTILITIES_HPP
