// The README examples, compiled and run by `ctest` so they can never rot.
// Builds as C++11.

#include "fastcast.hpp"

#include <cstdio>
#include <memory>
#include <typeinfo>

namespace {

struct Shape {
  virtual ~Shape() = default;
  virtual const char *name() const { return "shape"; }
};
struct Circle : Shape {
  const char *name() const override { return "circle"; }
  double radius = 1.0;
};
struct Square : Shape {
  const char *name() const override { return "square"; }
  double side = 2.0;
};

int failures = 0;
void expect(bool ok, const char *what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) {
    ++failures;
  }
}

} // namespace

int main() {
  Circle circle;
  Shape *shape = &circle;

  // --- Pointer cast: nullptr on failure, like dynamic_cast -----------------
  Circle *as_circle = fast_cast<Circle *>(shape);
  Square *as_square = fast_cast<Square *>(shape);
  expect(as_circle == &circle, "pointer downcast succeeds");
  expect(as_square == nullptr, "pointer downcast to the wrong type is null");

  // --- Reference cast: throws std::bad_cast on failure ---------------------
  Circle &ref = fast_cast<Circle &>(*shape);
  expect(&ref == &circle, "reference downcast succeeds");
  bool threw = false;
  try {
    Square &bad = fast_cast<Square &>(*shape);
    (void)bad;
  } catch (const std::bad_cast &) {
    threw = true;
  }
  expect(threw, "reference downcast to the wrong type throws std::bad_cast");

  // --- shared_ptr cast: shares ownership, empty on failure -----------------
  std::shared_ptr<Shape> owned = std::make_shared<Square>();
  std::shared_ptr<Square> square = fast_dynamic_pointer_cast<Square>(owned);
  std::shared_ptr<Circle> not_a_circle =
      fast_dynamic_pointer_cast<Circle>(owned);
  expect(square && square.use_count() == 2, "shared_ptr cast shares ownership");
  expect(!not_a_circle, "shared_ptr cast to the wrong type is empty");

  // --- Upcasts and identity casts cost nothing (resolved at compile time) --
  Shape *up = fast_cast<Shape *>(&circle);
  expect(up == shape, "upcast is a static_cast");

  // --- Casting to void* gives the most-derived object, like dynamic_cast ---
  expect(fast_cast<void *>(shape) == static_cast<void *>(&circle),
         "void* cast yields the most-derived object");

  // --- Repeated casts of the same dynamic type hit the cache ---------------
  for (int i = 0; i < 1000; ++i) {
    if (fast_cast<Circle *>(shape) != &circle) {
      ++failures;
    }
  }
  expect(true, "1000 cached casts");

  std::printf("%s (%s)\n", failures == 0 ? "all examples passed" : "FAILED",
              square->name());
  return failures == 0 ? 0 : 1;
}
