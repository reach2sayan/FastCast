//
// Created by sayan on 10/4/25.
//
// Unit tests for fastcast.hpp. Written against C++11 so that the same suite
// runs under every standard the library supports (the CI matrix covers
// C++11, 14, 17, 20 and 23 on GCC, Clang, AppleClang and MSVC).
//
// The tests mostly assert *parity with dynamic_cast*: whatever dynamic_cast
// returns for a given pointer, fast_cast must return the same thing, both the
// first time (cold cache) and every time after that (hot cache).

#define CATCH_CONFIG_MAIN
#include "../fastcast.hpp"
#include "catch.hpp"
#include "utilities.hpp"

#include <atomic>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

// ===========================================================================
// Version
// ===========================================================================

TEST_CASE("Version macros", "[version]") {
  CHECK(FASTCAST_VERSION_MAJOR == 1);
  CHECK(FASTCAST_VERSION_MINOR == 1);
  CHECK(FASTCAST_VERSION_PATCH == 0);
  CHECK(FASTCAST_VERSION == 10100);
  CHECK(std::string(FASTCAST_VERSION_STRING) == "1.1.0");

  // The numeric and string forms must agree with each other.
  const std::string composed = std::to_string(FASTCAST_VERSION_MAJOR) + "." +
                               std::to_string(FASTCAST_VERSION_MINOR) + "." +
                               std::to_string(FASTCAST_VERSION_PATCH);
  CHECK(composed == FASTCAST_VERSION_STRING);
  CHECK(FASTCAST_VERSION == FASTCAST_VERSION_MAJOR * 10000 +
                                FASTCAST_VERSION_MINOR * 100 +
                                FASTCAST_VERSION_PATCH);
}

TEST_CASE("Language version is detected", "[version]") {
  CHECK(FASTCAST_CPLUSPLUS >= 201103L);
#if defined(FASTCAST_SUPPORTED)
  INFO("cached fast path is enabled on this ABI");
  CHECK(true);
#else
  WARN("FASTCAST_SUPPORTED is not defined: every cast forwards to "
       "dynamic_cast on this ABI");
#endif
}

// ===========================================================================
// Compile-time properties
// ===========================================================================

namespace {

// fast_cast preserves the requested pointer / reference type exactly.
static_assert(
    std::is_same<decltype(fast_cast<Derived *>(std::declval<Base *>())),
                 Derived *>::value,
    "pointer result type");
static_assert(std::is_same<decltype(fast_cast<const Derived *>(
                               std::declval<const Base *>())),
                           const Derived *>::value,
              "const pointer result type");
static_assert(
    std::is_same<decltype(fast_cast<Derived &>(std::declval<Base &>())),
                 Derived &>::value,
    "reference result type");
static_assert(std::is_same<decltype(fast_cast<const Derived &>(
                               std::declval<const Base &>())),
                           const Derived &>::value,
              "const reference result type");
static_assert(std::is_same<decltype(fast_dynamic_pointer_cast<Derived>(
                               std::declval<std::shared_ptr<Base>>())),
                           std::shared_ptr<Derived>>::value,
              "shared_ptr result type");
static_assert(std::is_same<decltype(fast_cast<void *>(std::declval<Base *>())),
                           void *>::value,
              "void* result type");

// The pointer overload never throws; the reference overload may.
static_assert(noexcept(fast_cast<Derived *>(std::declval<Base *>())),
              "pointer cast is noexcept");
static_assert(!noexcept(fast_cast<Derived &>(std::declval<Base &>())),
              "reference cast may throw");
static_assert(noexcept(fast_dynamic_pointer_cast<Derived>(
                  std::declval<const std::shared_ptr<Base> &>())),
              "shared_ptr cast is noexcept");

// The static path is a constant expression in every supported standard.
static_assert(fast_cast<Base *>(static_cast<Derived *>(nullptr)) == nullptr,
              "derived-to-base is constexpr");
static_assert(fast_cast<Derived *>(static_cast<Derived *>(nullptr)) == nullptr,
              "identity is constexpr");
static_assert(fast_cast<const Base *>(static_cast<Derived *>(nullptr)) ==
                  nullptr,
              "const-adding is constexpr");
static_assert(fast_cast<PlainBase *>(static_cast<PlainDerived *>(nullptr)) ==
                  nullptr,
              "non-polymorphic derived-to-base is constexpr");

// Non-polymorphic types are fine as long as the cast is statically valid.
static_assert(std::is_same<decltype(fast_cast<PlainBase *>(
                               std::declval<PlainDerived *>())),
                           PlainBase *>::value,
              "non-polymorphic upcast compiles");

} // namespace

// ===========================================================================
// Static path: identity and derived-to-base
// ===========================================================================

TEST_CASE("Identity cast returns the same pointer", "[static]") {
  Derived d;
  Derived *dp = &d;
  CHECK(fast_cast<Derived *>(dp) == dp);

  const Derived *cdp = &d;
  CHECK(fast_cast<const Derived *>(cdp) == cdp);
  CHECK(fast_cast<const Derived *>(dp) == dp);
}

TEST_CASE("Derived-to-base uses static_cast", "[static]") {
  Derived d;
  Derived *dp = &d;
  CHECK(fast_cast<Base *>(dp) == static_cast<Base *>(dp));

  Multi m;
  CHECK(fast_cast<Base *>(&m) == static_cast<Base *>(&m));
  CHECK(fast_cast<AnotherBase *>(&m) == static_cast<AnotherBase *>(&m));
  // AnotherBase is not the primary base, so this one is a real adjustment.
  CHECK(static_cast<void *>(fast_cast<AnotherBase *>(&m)) !=
        static_cast<void *>(&m));
}

TEST_CASE("Derived-to-base reference", "[static][reference]") {
  Derived d;
  Base &b = fast_cast<Base &>(d);
  CHECK(&b == static_cast<Base *>(&d));
}

TEST_CASE("Non-polymorphic types work on the static path", "[static]") {
  PlainDerived pd;
  PlainBase *pb = fast_cast<PlainBase *>(&pd);
  CHECK(pb == &pd);
  CHECK(fast_cast<PlainDerived *>(&pd) == &pd);
}

// ===========================================================================
// Dynamic path: simple hierarchy
// ===========================================================================

TEST_CASE("SimpleHierarchy_DynamicVsFast", "[dynamic][reference]") {
  SimpleB b;
  SimpleA &a = b;
  SimpleB &db = dynamic_cast<SimpleB &>(a);
  CHECK(db.method_b_only() == 42);
  SimpleB &fb = fast_cast<SimpleB &>(a);
  CHECK(fb.method_b_only() == 42);
  CHECK(&db == &fb);
}

TEST_CASE("SimpleHierarchy_PointerNullCheck", "[dynamic]") {
  SimpleA *ap = nullptr;
  CHECK(dynamic_cast<SimpleB *>(ap) == nullptr);
  CHECK(fast_cast<SimpleB *>(ap) == nullptr);
}

TEST_CASE("Simple_Success_Ptr", "[dynamic]") {
  SimpleB b;
  SimpleA *a = &b;

  SimpleB *bp = dynamic_cast<SimpleB *>(a);
  SimpleB *fp = fast_cast<SimpleB *>(a);

  REQUIRE(bp != nullptr);
  REQUIRE(fp != nullptr);
  CHECK(bp == fp);
  CHECK(bp->method_b_only() == 42);
  CHECK(fp->method_b_only() == 42);
}

TEST_CASE("Simple_Failure_PtrNull", "[dynamic]") {
  std::unique_ptr<SimpleA> a(new SimpleA);
  CHECK(dynamic_cast<SimpleB *>(a.get()) == nullptr);
  CHECK(fast_cast<SimpleB *>(a.get()) == nullptr);
}

TEST_CASE("Simple_Failure_BadCastRef", "[dynamic][reference]") {
  SimpleA a;
  CHECK_THROWS_AS((void)dynamic_cast<SimpleB &>(a), std::bad_cast);
  CHECK_THROWS_AS((void)fast_cast<SimpleB &>(a), std::bad_cast);
}

TEST_CASE("Simple_ConstCorrectness", "[dynamic][const]") {
  const SimpleB b;
  const SimpleA &a = b;
  const SimpleB &br = dynamic_cast<const SimpleB &>(a);
  const SimpleB &fr = fast_cast<const SimpleB &>(a);
  CHECK(&br == &fr);
  CHECK(br.method_b_only() == fr.method_b_only());

  const SimpleA *ap = &b;
  CHECK(fast_cast<const SimpleB *>(ap) == &b);
}

TEST_CASE("BaseToDerived_RuntimePath", "[dynamic]") {
  Derived d;
  Base *bp = &d;
  Derived *dp = fastcast::fast_cast<Derived *>(bp);
  CHECK(dp == dynamic_cast<Derived *>(bp));
  CHECK(dp == &d);
}

// ===========================================================================
// Dynamic path: complex hierarchy (virtual + multiple inheritance)
// ===========================================================================

TEST_CASE("ComplexHierarchy_DynamicVsFast", "[dynamic][reference]") {
  ComplexG g;
  ComplexA &a = g;
  ComplexG &dg = dynamic_cast<ComplexG &>(a);
  ComplexG &fg = fast_cast<ComplexG &>(a);
  CHECK(&dg == &fg);
  CHECK(dg.method_g_only() == 1729);
  CHECK(fg.method_g_only() == 1729);
}

TEST_CASE("ComplexHierarchy_PointerNullCheck", "[dynamic]") {
  ComplexA *ap = nullptr;
  CHECK(dynamic_cast<ComplexG *>(ap) == nullptr);
  CHECK(fast_cast<ComplexG *>(ap) == nullptr);
}

TEST_CASE("ComplexHierarchy_CrossCast", "[dynamic]") {
  ComplexG g;
  ComplexA &a = g;
  // Cross-cast down to ComplexF through ComplexA
  ComplexF *df = dynamic_cast<ComplexF *>(&a);
  ComplexF *ff = fast_cast<ComplexF *>(&a);

  REQUIRE(df != nullptr);
  REQUIRE(ff != nullptr);
  CHECK(df == ff);
  CHECK(df->method() == ff->method());
}

TEST_CASE("Complex_Success_Ref", "[dynamic][reference]") {
  ComplexE w;
  ComplexA &x = w;

  ComplexE &dr = dynamic_cast<ComplexE &>(x);
  ComplexE &fr = fast_cast<ComplexE &>(x);

  CHECK(&dr == &fr);
  CHECK(dr.method() == 2520);
  CHECK(fr.method() == 2520);
}

TEST_CASE("Complex_Success_CrossCast", "[dynamic]") {
  ComplexE w;
  ComplexA *x = &w;

  ComplexB *dz = dynamic_cast<ComplexB *>(x);
  ComplexB *fz = fast_cast<ComplexB *>(x);

  REQUIRE(dz != nullptr);
  REQUIRE(fz != nullptr);
  CHECK(dz == fz);
  CHECK(dz->method() == fz->method());
}

TEST_CASE("Complex_Failure_NullPtr", "[dynamic]") {
  ComplexA *x = nullptr;
  CHECK(dynamic_cast<ComplexF *>(x) == nullptr);
  CHECK(fast_cast<ComplexF *>(x) == nullptr);
}

TEST_CASE("Complex_Failure_Unrelated", "[dynamic][reference]") {
  // A ComplexE is neither a ComplexD nor a ComplexF nor a ComplexG.
  ComplexE w;
  ComplexA &a = w;
  CHECK_THROWS_AS((void)dynamic_cast<ComplexG &>(a), std::bad_cast);
  CHECK_THROWS_AS((void)fast_cast<ComplexG &>(a), std::bad_cast);
  CHECK_THROWS_AS((void)fast_cast<ComplexF &>(a), std::bad_cast);
  CHECK_THROWS_AS((void)fast_cast<ComplexD &>(a), std::bad_cast);
  CHECK(fast_cast<ComplexD *>(&a) == nullptr);
}

TEST_CASE("Every base of ComplexG is reachable from every other base",
          "[dynamic]") {
  ComplexG g;
  ComplexA *a = &g;
  ComplexB *b = &g;
  ComplexC *c = &g;
  ComplexD *d = &g;
  ComplexE *e = &g;
  ComplexF *f = &g;

  // From the primary path (A/B/C/D) to the secondary path (E/F) and back.
  CHECK(fast_cast<ComplexE *>(d) == e);
  CHECK(fast_cast<ComplexF *>(c) == f);
  CHECK(fast_cast<ComplexD *>(f) == d);
  CHECK(fast_cast<ComplexC *>(e) == c);
  CHECK(fast_cast<ComplexG *>(f) == &g);
  CHECK(fast_cast<ComplexG *>(e) == &g);

  // Down to the virtual base from every subobject (no static_cast possible).
  CHECK(fast_cast<ComplexB *>(f) == b);
  CHECK(fast_cast<ComplexB *>(d) == b);
  CHECK(fast_cast<ComplexA *>(f) == a);

  // Repeat once more so every pair above also exercises the hot cache.
  CHECK(fast_cast<ComplexE *>(d) == e);
  CHECK(fast_cast<ComplexF *>(c) == f);
  CHECK(fast_cast<ComplexD *>(f) == d);
  CHECK(fast_cast<ComplexC *>(e) == c);
  CHECK(fast_cast<ComplexG *>(f) == &g);
  CHECK(fast_cast<ComplexB *>(f) == b);
}

// ===========================================================================
// Multiple inheritance: non-zero offsets and cross-casts
// ===========================================================================

TEST_CASE("MultipleInheritance_CrossCast", "[dynamic]") {
  Multi m;
  Base *bp = &m;
  AnotherBase *ap = fastcast::fast_cast<AnotherBase *>(bp);
  CHECK(ap == dynamic_cast<AnotherBase *>(bp));
  CHECK(ap == static_cast<AnotherBase *>(&m));
  REQUIRE(ap != nullptr);
}

TEST_CASE("Downcast from a non-primary base adjusts the pointer", "[dynamic]") {
  Multi m;
  AnotherBase *ap = &m;
  REQUIRE(static_cast<void *>(ap) != static_cast<void *>(&m));

  CHECK(fast_cast<Multi *>(ap) == &m);
  CHECK(fast_cast<Multi *>(ap) == dynamic_cast<Multi *>(ap));
  CHECK(fast_cast<Base *>(ap) == static_cast<Base *>(&m));

  // And again from the hot cache.
  CHECK(fast_cast<Multi *>(ap) == &m);
  CHECK(fast_cast<Base *>(ap) == static_cast<Base *>(&m));
}

TEST_CASE("MultipleInheritance_Failure", "[dynamic]") {
  // A plain Derived has no AnotherBase.
  Derived d;
  Base *bp = &d;
  CHECK(fast_cast<AnotherBase *>(bp) == nullptr);
  CHECK(fast_cast<AnotherBase *>(bp) == dynamic_cast<AnotherBase *>(bp));
  CHECK(fast_cast<Multi *>(bp) == nullptr);
}

TEST_CASE("Two subobjects of the same base type are told apart",
          "[dynamic][cache]") {
  TwoBases t;
  Base *left = static_cast<LeftBase *>(&t);
  Base *right = static_cast<RightBase *>(&t);
  REQUIRE(left != right);

  // Same (From, To) pair, different vtable pointers, different offsets.
  CHECK(fast_cast<TwoBases *>(left) == &t);
  CHECK(fast_cast<TwoBases *>(right) == &t);
  CHECK(fast_cast<TwoBases *>(left) == &t);
  CHECK(fast_cast<TwoBases *>(right) == &t);

  CHECK(fast_cast<LeftBase *>(left) == static_cast<LeftBase *>(&t));
  CHECK(fast_cast<RightBase *>(right) == static_cast<RightBase *>(&t));
  // dynamic_cast<LeftBase*> from the *right* Base subobject is a cross-cast
  // that succeeds (LeftBase is an unambiguous base of the most-derived
  // TwoBases). Whatever dynamic_cast says, fast_cast must agree.
  CHECK(fast_cast<LeftBase *>(right) == dynamic_cast<LeftBase *>(right));
  CHECK(fast_cast<RightBase *>(left) == dynamic_cast<RightBase *>(left));
}

// ===========================================================================
// void* : most-derived object address
// ===========================================================================

TEST_CASE("Cast to void* yields the most-derived object", "[dynamic][void]") {
  Multi m;
  AnotherBase *ap = &m;
  Base *bp = &m;

  CHECK(fast_cast<void *>(ap) == dynamic_cast<void *>(ap));
  CHECK(fast_cast<void *>(ap) == static_cast<void *>(&m));
  CHECK(fast_cast<void *>(bp) == static_cast<void *>(&m));
  // hot path
  CHECK(fast_cast<void *>(ap) == static_cast<void *>(&m));

  const AnotherBase *cap = &m;
  CHECK(fast_cast<const void *>(cap) == dynamic_cast<const void *>(cap));

  ComplexG g;
  ComplexF *f = &g;
  CHECK(fast_cast<void *>(f) == dynamic_cast<void *>(f));
  CHECK(fast_cast<void *>(f) == static_cast<void *>(&g));

  void *np = fast_cast<void *>(static_cast<Base *>(nullptr));
  CHECK(np == nullptr);
}

// ===========================================================================
// shared_ptr
// ===========================================================================

TEST_CASE("shared_ptr cast shares ownership", "[shared_ptr]") {
  std::shared_ptr<Base> sb = std::make_shared<Derived>();
  REQUIRE(sb.use_count() == 1);

  std::shared_ptr<Derived> sd = fast_dynamic_pointer_cast<Derived>(sb);
  REQUIRE(sd);
  CHECK(sd.get() == std::dynamic_pointer_cast<Derived>(sb).get());
  CHECK(sd.get() == static_cast<Derived *>(sb.get()));
  CHECK(sb.use_count() == 2);
  CHECK(sd.use_count() == 2);

  // The alias keeps the object alive after the original goes away.
  sb.reset();
  CHECK(sd.use_count() == 1);
  CHECK(sd.get() != nullptr);
}

TEST_CASE("shared_ptr cast failure yields an empty pointer", "[shared_ptr]") {
  std::shared_ptr<Base> sb = std::make_shared<Base>();
  std::shared_ptr<Derived> sd = fast_dynamic_pointer_cast<Derived>(sb);
  CHECK(!sd);
  CHECK(sd.use_count() == 0);
  CHECK(sb.use_count() == 1);
}

TEST_CASE("shared_ptr cast of an empty pointer", "[shared_ptr]") {
  std::shared_ptr<Base> empty;
  CHECK(!fast_dynamic_pointer_cast<Derived>(empty));
  CHECK(!fast_dynamic_pointer_cast<Base>(empty));
}

TEST_CASE("shared_ptr cross-cast and upcast", "[shared_ptr]") {
  std::shared_ptr<Base> sb = std::make_shared<Multi>();
  std::shared_ptr<AnotherBase> sa = fast_dynamic_pointer_cast<AnotherBase>(sb);
  REQUIRE(sa);
  CHECK(sa.get() == std::dynamic_pointer_cast<AnotherBase>(sb).get());
  CHECK(sb.use_count() == 2);

  std::shared_ptr<Multi> sm = fast_dynamic_pointer_cast<Multi>(sa);
  REQUIRE(sm);
  CHECK(sm.get() == std::dynamic_pointer_cast<Multi>(sa).get());
  CHECK(sb.use_count() == 3);

  // Upcast (static path) through the shared_ptr overload.
  std::shared_ptr<Base> back = fast_dynamic_pointer_cast<Base>(sm);
  CHECK(back.get() == sb.get());
  CHECK(sb.use_count() == 4);
}

TEST_CASE("shared_ptr to const", "[shared_ptr][const]") {
  std::shared_ptr<const Base> sb = std::make_shared<Derived>();
  std::shared_ptr<const Derived> sd =
      fast_dynamic_pointer_cast<const Derived>(sb);
  REQUIRE(sd);
  CHECK(sd.get() == sb.get());
}

// ===========================================================================
// Cache behaviour
// ===========================================================================

TEST_CASE("FailureCaching", "[cache]") {
  Base b;
  Base *bp = &b;
  Derived *dp = fastcast::fast_cast<Derived *>(bp);
  CHECK(dp == nullptr);

  // call again, should hit failure cache fast
  Derived *dp2 = fastcast::fast_cast<Derived *>(bp);
  CHECK(dp2 == nullptr);
}

TEST_CASE("Cache alternates correctly between failure and success", "[cache]") {
  Base plain;
  Derived derived;
  Base *bp = &plain;
  Base *dp = &derived;

  for (int i = 0; i < 4; ++i) {
    CHECK(fast_cast<Derived *>(bp) == nullptr);  // miss -> cached miss
    CHECK(fast_cast<Derived *>(dp) == &derived); // new vtable -> success
    CHECK(fast_cast<Derived *>(dp) == &derived); // hot success
    CHECK(fast_cast<Derived *>(bp) == nullptr);  // new vtable -> miss again
  }
}

TEST_CASE("Cache is per (From, To) pair", "[cache]") {
  ComplexG g;
  ComplexA *a = &g;

  // Interleave several different targets from the same source; each pair
  // owns its own cache entry, so they must not disturb one another.
  for (int i = 0; i < 3; ++i) {
    CHECK(fast_cast<ComplexB *>(a) == dynamic_cast<ComplexB *>(a));
    CHECK(fast_cast<ComplexC *>(a) == dynamic_cast<ComplexC *>(a));
    CHECK(fast_cast<ComplexD *>(a) == dynamic_cast<ComplexD *>(a));
    CHECK(fast_cast<ComplexE *>(a) == dynamic_cast<ComplexE *>(a));
    CHECK(fast_cast<ComplexF *>(a) == dynamic_cast<ComplexF *>(a));
    CHECK(fast_cast<ComplexG *>(a) == dynamic_cast<ComplexG *>(a));
  }
}

TEST_CASE("Cached offset applies to any object of the same dynamic type",
          "[cache]") {
  // Warm the cache with one object, then cast many others at different
  // addresses: the stored offset is relative, so all must be correct.
  std::vector<std::unique_ptr<ComplexG>> objs;
  for (int i = 0; i < 64; ++i) {
    objs.emplace_back(new ComplexG);
  }
  for (std::size_t i = 0; i < objs.size(); ++i) {
    ComplexA *a = objs[i].get();
    ComplexF *f = objs[i].get();
    CHECK(fast_cast<ComplexG *>(a) == objs[i].get());
    CHECK(fast_cast<ComplexF *>(a) == f);
    CHECK(fast_cast<ComplexG *>(f) == objs[i].get());
    CHECK(fast_cast<ComplexB *>(f) == static_cast<ComplexB *>(objs[i].get()));
  }
}

TEST_CASE("Cache churn across many dynamic types", "[cache]") {
  // Rotating through many distinct leaf types through one (From, To) pair
  // forces a cache miss on every call; results must still be exact.
  Leaf<0> l0;
  Leaf<1> l1;
  Leaf<2> l2;
  Leaf<3> l3;
  Leaf<4> l4;
  Leaf<5> l5;
  Leaf<6> l6;
  Leaf<7> l7;
  Base *objs[] = {&l0, &l1, &l2, &l3, &l4, &l5, &l6, &l7};
  Base plain;

  for (int round = 0; round < 4; ++round) {
    CHECK(fast_cast<Leaf<3> *>(objs[0]) == nullptr);
    CHECK(fast_cast<Leaf<3> *>(objs[3]) == &l3);
    CHECK(fast_cast<Leaf<3> *>(objs[7]) == nullptr);
    CHECK(fast_cast<Leaf<3> *>(&plain) == nullptr);
    CHECK(fast_cast<Leaf<3> *>(objs[3]) == &l3);
    for (int i = 0; i < 8; ++i) {
      CHECK(fast_cast<Leaf<5> *>(objs[i]) == dynamic_cast<Leaf<5> *>(objs[i]));
      CHECK(fast_cast<Leaf<0> *>(objs[i]) == dynamic_cast<Leaf<0> *>(objs[i]));
    }
  }
  CHECK(fast_cast<Leaf<5> *>(objs[5])->id() == 5);
}

namespace {

struct ProbeDerived;

// Casts `this` to the derived type from inside the base constructor, where
// the dynamic type is still the base. dynamic_cast returns nullptr there;
// fast_cast must too, and must not poison the cache for the completed object.
struct ProbeBase {
  ProbeBase();
  virtual ~ProbeBase() = default;
  bool saw_derived_in_ctor;
};
struct ProbeDerived : ProbeBase {};
ProbeBase::ProbeBase()
    : saw_derived_in_ctor(fast_cast<ProbeDerived *>(this) != nullptr) {}

} // namespace

TEST_CASE("Casting during construction matches dynamic_cast", "[cache]") {
  ProbeDerived d;
  CHECK_FALSE(d.saw_derived_in_ctor);
  ProbeBase *bp = &d;
  CHECK(fast_cast<ProbeDerived *>(bp) == &d);
  CHECK(fast_cast<ProbeDerived *>(bp) == dynamic_cast<ProbeDerived *>(bp));

  ProbeDerived d2; // constructor runs the cast again with the base vtable
  CHECK_FALSE(d2.saw_derived_in_ctor);
  CHECK(fast_cast<ProbeDerived *>(bp) == &d);
  CHECK(fast_cast<ProbeDerived *>(&d2) == &d2);
}

// ===========================================================================
// Randomised parity test against dynamic_cast
// ===========================================================================

namespace {

struct Pool {
  std::vector<std::unique_ptr<ComplexA>> objs;
  Pool() {
    for (int i = 0; i < 4; ++i) {
      objs.emplace_back(new ComplexA);
      objs.emplace_back(new ComplexB);
      objs.emplace_back(new ComplexC);
      objs.emplace_back(new ComplexD);
      objs.emplace_back(new ComplexE);
      objs.emplace_back(new ComplexF);
      objs.emplace_back(new ComplexG);
    }
  }
};

// Compares fast_cast with dynamic_cast for every target type; returns the
// number of disagreements.
int check_all_targets(ComplexA *a) {
  int bad = 0;
  bad += fast_cast<ComplexA *>(a) != dynamic_cast<ComplexA *>(a);
  bad += fast_cast<ComplexB *>(a) != dynamic_cast<ComplexB *>(a);
  bad += fast_cast<ComplexC *>(a) != dynamic_cast<ComplexC *>(a);
  bad += fast_cast<ComplexD *>(a) != dynamic_cast<ComplexD *>(a);
  bad += fast_cast<ComplexE *>(a) != dynamic_cast<ComplexE *>(a);
  bad += fast_cast<ComplexF *>(a) != dynamic_cast<ComplexF *>(a);
  bad += fast_cast<ComplexG *>(a) != dynamic_cast<ComplexG *>(a);
  bad += fast_cast<void *>(a) != dynamic_cast<void *>(a);
  return bad;
}

} // namespace

TEST_CASE("Random cast sequences agree with dynamic_cast", "[cache][fuzz]") {
  Pool pool;
  std::mt19937 rng(20251004u); // fixed seed: deterministic
  std::uniform_int_distribution<std::size_t> pick(0, pool.objs.size() - 1);

  int mismatches = 0;
  for (int i = 0; i < 20000; ++i) {
    mismatches += check_all_targets(pool.objs[pick(rng)].get());
  }
  CHECK(mismatches == 0);
}

// ===========================================================================
// Threads
// ===========================================================================

TEST_CASE("Concurrent casts from many threads are correct", "[threads]") {
  Pool pool;
  const unsigned hw = std::thread::hardware_concurrency();
  const unsigned nthreads = hw == 0 ? 4u : (hw < 8u ? hw : 8u);
  const int iterations = 20000;

  std::atomic<int> mismatches(0);
  std::vector<std::thread> threads;
  for (unsigned t = 0; t < nthreads; ++t) {
    threads.emplace_back([&pool, &mismatches, t]() {
      // Each thread walks the pool with a different stride so that the
      // per-thread caches see different sequences of dynamic types.
      const std::size_t n = pool.objs.size();
      std::size_t idx = t % n;
      int local = 0;
      for (int i = 0; i < iterations; ++i) {
        local += check_all_targets(pool.objs[idx].get());
        idx = (idx + 1 + t) % n;
      }
      mismatches += local;
    });
  }
  for (std::size_t i = 0; i < threads.size(); ++i) {
    threads[i].join();
  }
  CHECK(mismatches.load() == 0);
}

TEST_CASE("Objects created on one thread cast correctly on another",
          "[threads]") {
  std::shared_ptr<ComplexA> made_elsewhere;
  std::thread producer([&made_elsewhere]() {
    made_elsewhere = std::make_shared<ComplexG>();
    // Warm this thread's cache; the main thread's cache is separate.
    (void)fast_cast<ComplexG *>(made_elsewhere.get());
  });
  producer.join();

  REQUIRE(made_elsewhere);
  CHECK(fast_cast<ComplexG *>(made_elsewhere.get()) ==
        dynamic_cast<ComplexG *>(made_elsewhere.get()));
  CHECK(fast_dynamic_pointer_cast<ComplexF>(made_elsewhere).get() ==
        std::dynamic_pointer_cast<ComplexF>(made_elsewhere).get());
}

// ===========================================================================
// Namespace / API surface
// ===========================================================================

TEST_CASE("Functions are reachable through the fastcast namespace", "[api]") {
  Derived d;
  Base *bp = &d;
  CHECK(fastcast::fast_cast<Derived *>(bp) == &d);
  CHECK(&fastcast::fast_cast<Derived &>(*bp) == &d);
  std::shared_ptr<Base> sb = std::make_shared<Derived>();
  CHECK(fastcast::fast_dynamic_pointer_cast<Derived>(sb).get() == sb.get());
}
