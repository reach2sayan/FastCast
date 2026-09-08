// Expected diagnostic: "cannot cast away const"
#include "fastcast.hpp"

struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};

int main() {
  Derived d;
  const Base &cb = d;
  Derived &r = fast_cast<Derived &>(cb); // const Base& -> Derived&: rejected
  return &r == &d ? 0 : 1;
}
