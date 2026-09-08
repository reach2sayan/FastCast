// Expected diagnostic: "no matching" (no overload accepts a by-value target)
#include "fastcast.hpp"

struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};

int main() {
  Derived d;
  Base *bp = &d;
  Derived copy =
      fast_cast<Derived>(bp); // target must be a pointer or reference
  return &copy == &d ? 0 : 1;
}
