// Expected diagnostic: "cannot cast away const"
#include "fastcast.hpp"

struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};

int main() {
  const Base *cb = nullptr;
  Derived *d = fast_cast<Derived *>(cb); // const Base* -> Derived*: rejected
  return d != nullptr;
}
