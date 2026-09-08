// Uses the installed header via find_package(FastCast); see CMakeLists.txt.
#include <fastcast.hpp>

struct Base {
  virtual ~Base() = default;
};
struct Derived : Base {};

int main() {
  Derived d;
  Base *b = &d;
  static_assert(FASTCAST_VERSION_MAJOR == 1, "major version");
  return fast_cast<Derived *>(b) == &d ? 0 : 1;
}
