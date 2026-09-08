// Expected diagnostic: "not polymorphic"
#include "fastcast.hpp"

struct PlainBase {
  int x = 0;
};
struct PlainDerived : PlainBase {};

int main() {
  PlainBase b;
  PlainBase *pb = &b;
  // A downcast needs RTTI, which a non-polymorphic type does not have.
  PlainDerived *pd = fast_cast<PlainDerived *>(pb);
  return pd != nullptr;
}
