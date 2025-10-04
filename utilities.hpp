//
// Created by sayan on 10/4/25.
//

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

//
// Complex diamond-ish hierarchy used in your test
//
//      A
//      |
//      B
//      | \
//      C  E
//      |  |
//      D  F
//       \/
//        G
//
struct ComplexA {
  virtual ~ComplexA() = default;
  virtual int method() { return 1; }
};
struct ComplexB : public ComplexA {
  virtual int method() override { return 2; }
};
struct ComplexC : public virtual ComplexB {
  virtual int method() override { return 3; }
};
struct ComplexD : public ComplexC {
  virtual int method() override { return 4; }
};
struct ComplexE : public virtual ComplexB {
  virtual int method() override { return 2520; }
};
struct ComplexF : public ComplexE {
  virtual int method() override { return 6; }
};
struct ComplexG : public ComplexD, public ComplexF {
  virtual int method() override { return 1729; }
  virtual int method_g_only() { return method(); }
};

struct Base { virtual ~Base() = default; };
struct Derived : Base {};
struct AnotherBase { virtual ~AnotherBase() = default; };
struct Multi : Base, AnotherBase {};

#endif // FASTCAST_UTILITIES_HPP
