<!-- status -->
[![CI](https://github.com/reach2sayan/FastCast/actions/workflows/action.yml/badge.svg?branch=main)](https://github.com/reach2sayan/FastCast/actions/workflows/action.yml)
[![Release](https://github.com/reach2sayan/FastCast/actions/workflows/release.yml/badge.svg)](https://github.com/reach2sayan/FastCast/actions/workflows/release.yml)
[![Latest release](https://img.shields.io/github/v/release/reach2sayan/FastCast?sort=semver&display_name=tag&logo=github)](https://github.com/reach2sayan/FastCast/releases/latest)
[![NuGet](https://img.shields.io/nuget/v/FastCast?logo=nuget)](https://www.nuget.org/packages/FastCast)
[![NuGet downloads](https://img.shields.io/nuget/dt/FastCast?logo=nuget&label=nuget%20downloads)](https://www.nuget.org/packages/FastCast)
[![Release downloads](https://img.shields.io/github/downloads/reach2sayan/FastCast/total?logo=github&label=release%20downloads)](https://github.com/reach2sayan/FastCast/releases)
<!-- facts -->
[![C++11 … C++23](https://img.shields.io/badge/C%2B%2B-11%20%E2%80%A6%2023-%2300599C.svg?logo=c%2B%2B&logoColor=white)](#language-and-compiler-support)
[![Header-only](https://img.shields.io/badge/header--only-single%20file-brightgreen.svg)](fastcast.hpp)
[![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](#language-and-compiler-support)
[![Compilers](https://img.shields.io/badge/compilers-GCC%20%7C%20Clang%20%7C%20AppleClang%20%7C%20MSVC-lightgrey.svg)](#language-and-compiler-support)
[![Sanitizers](https://img.shields.io/badge/tested%20with-ASan%20%7C%20UBSan-blueviolet.svg)](.github/workflows/action.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.txt)

# FastCast

**`fast_cast`** is a header-only, C++11-compatible, drop-in replacement for
`dynamic_cast` that caches the result of each cast per thread. Repeated casts of
the same dynamic type become a single pointer comparison plus an addition, which is
**35–50× faster than `dynamic_cast`** on a hot path, while returning exactly what
`dynamic_cast` would return.

```cpp
#include "fastcast.hpp"

Shape *shape = get_shape();
Circle *c = fast_cast<Circle *>(shape);      // nullptr if it is not a Circle
Circle &r = fast_cast<Circle &>(*shape);     // throws std::bad_cast if not
auto sp  = fast_dynamic_pointer_cast<Circle>(shared_shape);  // empty on failure
```

The code is heavily inspired by [FastDynamicCast](https://github.com/tobspr/FastDynamicCast)
and is effectively a modern, portable reimplementation of it. The basic idea in one picture:

![offset](ptroffset.png)

`dynamic_cast` walks the RTTI graph every time. But for a given *dynamic type* (identified by
its vtable pointer) the answer never changes: the target subobject is always at the same
byte offset from the source subobject, or the cast always fails. `fast_cast` remembers that
offset the first time it sees a vtable and reuses it on every subsequent call. Also read
[this reddit post](https://www.reddit.com/r/cpp/comments/ilbf1y/vtable_layout_differences_between_itanium_and)
on why this works on both the Itanium and the MSVC ABI.

## Contents

- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [API reference](#api-reference)
- [Semantics: how it compares to `dynamic_cast`](#semantics-how-it-compares-to-dynamic_cast)
- [How it works](#how-it-works)
- [Caveats](#caveats)
- [Language and compiler support](#language-and-compiler-support)
- [Benchmarks](#benchmarks)
- [Building and testing](#building-and-testing)
- [Releasing](#releasing)
- [Contributing](#contributing)
- [License](#license)

## Features

- **Drop-in**: pointer, reference, and `std::shared_ptr` overloads with the same results,
  null/throw behaviour and const-correctness as `dynamic_cast` / `std::dynamic_pointer_cast`.
- **Compile-time fast paths**: identity, cv-adding and derived-to-base casts compile to a
  `static_cast` (or nothing at all) and are `constexpr` even in C++11.
- **Runtime fast path**: per-thread, per-`(From, To)` cache of the last vtable seen and the
  pointer adjustment it needed. A cache hit is a load, a compare and an add.
- **Failed casts are cached too**, so repeated misses are as cheap as repeated hits.
- **Works with anything `dynamic_cast` works with**: multiple inheritance, virtual
  inheritance, cross-casts between sibling bases, casts to `void*` (most-derived object).
- **Header-only, C++11 and up**, no dependencies, no linking. Only `<type_traits>`,
  `<memory>`, `<cstring>`, `<limits>` and `<typeinfo>` are included.
- **Portable**: the cached path is used on the Itanium ABI (GCC, Clang, ICC, AppleClang,
  MinGW) and on MSVC. On any other ABI every call transparently forwards to `dynamic_cast`.
- **Thread-safe** without locks or atomics: each thread owns its own cache.

## Installation

### Copy the header

Everything is in [`fastcast.hpp`](fastcast.hpp). Copy it into your project and

```cpp
#include "fastcast.hpp"
```

### CMake: `add_subdirectory` / `FetchContent`

```cmake
include(FetchContent)
FetchContent_Declare(FastCast
        GIT_REPOSITORY https://github.com/reach2sayan/FastCast.git
        GIT_TAG        main)   # or a release tag
FetchContent_MakeAvailable(FastCast)

target_link_libraries(my_target PRIVATE FastCast::FastCast)
```

Tests, examples and benchmarks are only built when FastCast is the top-level project, so
consuming it this way never pulls in Google Benchmark.

### CMake: install and `find_package`

```sh
cmake -S FastCast -B build -DFASTCAST_BUILD_TESTS=OFF -DFASTCAST_BUILD_EXAMPLES=OFF \
      -DFASTCAST_BUILD_BENCHMARKS=OFF -DCMAKE_INSTALL_PREFIX=/opt/fastcast
cmake --build build && cmake --install build
```

```cmake
find_package(FastCast 1.1 REQUIRED)
target_link_libraries(my_target PRIVATE FastCast::FastCast)
```

The header is installed to `<prefix>/include/fastcast.hpp`, so use `#include <fastcast.hpp>`.

### NuGet (Visual Studio / MSBuild)

The [`FastCast`](https://www.nuget.org/packages/FastCast) package carries the header and a
`.targets` file that adds the include path to every project referencing it. It is
header-only, so it is valid for any platform, architecture and configuration:

```
Install-Package FastCast
```

or in the `.vcxproj` / `packages.config`:

```xml
<PackageReference Include="FastCast" Version="1.1.0" />
```

Every GitHub release also attaches the `.nupkg`, for private feeds.

### Downloadable tarball

Every [GitHub release](https://github.com/reach2sayan/FastCast/releases) attaches:

| Asset | Contents |
|---|---|
| `FastCast-<version>.tar.gz` / `.zip` | Installable tree: `include/fastcast.hpp`, `lib/cmake/FastCast/` (for `find_package`), `share/doc/FastCast/`. Unpack it anywhere and point `CMAKE_PREFIX_PATH` at it. |
| `FastCast-<version>-src.tar.gz` | Full source tree with tests, examples and benchmarks. |
| `FastCast.<version>.nupkg` | The NuGet package. |
| `fastcast.hpp` | Just the header. |

## Usage

All examples below are compiled and run by `ctest` from [`examples/basic.cpp`](examples/basic.cpp).

### Pointer casting

```cpp
struct Shape  { virtual ~Shape() = default; };
struct Circle : Shape {};
struct Square : Shape {};

Circle circle;
Shape *shape = &circle;

Circle *c = fast_cast<Circle *>(shape);   // == &circle
Square *s = fast_cast<Square *>(shape);   // == nullptr
```

### Reference casting

```cpp
Circle &c = fast_cast<Circle &>(*shape);  // ok
try {
  Square &s = fast_cast<Square &>(*shape);
} catch (const std::bad_cast &) {
  // exactly like dynamic_cast<Square &>
}
```

### `std::shared_ptr` casting

```cpp
std::shared_ptr<Shape> owned = std::make_shared<Square>();

std::shared_ptr<Square> square = fast_dynamic_pointer_cast<Square>(owned);  // shares ownership
std::shared_ptr<Circle> circle = fast_dynamic_pointer_cast<Circle>(owned);  // empty
```

### Upcasts, identity casts and `const`

```cpp
Shape *up = fast_cast<Shape *>(&circle);          // static_cast, zero cost, constexpr
Circle *same = fast_cast<Circle *>(&circle);      // identity, zero cost, constexpr

const Shape *cs = &circle;
const Circle *cc = fast_cast<const Circle *>(cs); // fine
Circle *bad = fast_cast<Circle *>(cs);            // compile error: cannot cast away const
```

### Most-derived object (`void*`)

```cpp
void *whole = fast_cast<void *>(shape);  // same as dynamic_cast<void *>(shape)
```

### Opting out of the global names

The header exports `fast_cast` and `fast_dynamic_pointer_cast` into the global namespace
for convenience. Define `FASTCAST_NO_GLOBAL_USING` before including it to keep them in
`namespace fastcast` only:

```cpp
#define FASTCAST_NO_GLOBAL_USING
#include "fastcast.hpp"
auto *c = fastcast::fast_cast<Circle *>(shape);
```

## API reference

Everything lives in `namespace fastcast`.

| Function | Result on failure | Notes |
|---|---|---|
| `template <class To, class From> To fast_cast(From *p) noexcept` | `nullptr` | `To` must be a pointer type. `constexpr` when the cast is static. |
| `template <class To, class From> To fast_cast(From &r)` | throws `std::bad_cast` | `To` must be a reference type. `constexpr` from C++14 when the cast is static. |
| `template <class To, class From> std::shared_ptr<To> fast_dynamic_pointer_cast(const std::shared_ptr<From> &p) noexcept` | empty `shared_ptr` | Result shares ownership with `p` (aliasing constructor), like `std::dynamic_pointer_cast`. |

Macros:

| Macro | Meaning |
|---|---|
| `FASTCAST_VERSION_MAJOR` / `_MINOR` / `_PATCH` | Version components. |
| `FASTCAST_VERSION` | `MAJOR * 10000 + MINOR * 100 + PATCH`, e.g. `10100` for 1.1.0. |
| `FASTCAST_VERSION_STRING` | `"1.1.0"`. |
| `FASTCAST_CPLUSPLUS` | Detected language version (`__cplusplus`, or `_MSVC_LANG` on MSVC). |
| `FASTCAST_SUPPORTED` | Defined (to `1`) when the cached fast path is active for this ABI. Undefined means every cast forwards to `dynamic_cast`. |
| `FASTCAST_NO_GLOBAL_USING` | Define before including to suppress the global `using` declarations. |

## Semantics: how it compares to `dynamic_cast`

For every well-formed cast, `fast_cast<T>(x)` returns the same value as `dynamic_cast<T>(x)`
(or throws the same exception). The test suite asserts this by comparing the two side by side
on simple, multiple, virtual and diamond hierarchies, cold and hot, from many threads, and
under a randomised sequence of dynamic types. The differences are all compile-time:

| Cast | `dynamic_cast` | `fast_cast` |
|---|---|---|
| Downcast / cross-cast of a polymorphic type | runtime check | runtime check, cached |
| Identity, cv-adding, derived-to-base | runtime no-op | `static_cast`, `constexpr` |
| To `void*` | most-derived object | most-derived object (cached) |
| Non-polymorphic source, downcast | compile error | compile error (`static_assert`) |
| Casting away `const` | compile error | compile error (`static_assert`) |
| `volatile`-qualified pointee, downcast | allowed | not supported |
| From inside a base-class constructor | sees the base's dynamic type | same |

## How it works

`fast_cast<To>(From *p)` is resolved at compile time into one of two paths:

1. **Static path** — if `From*` converts implicitly to `To` (identity, adding `const`,
   derived-to-base) the call is a `static_cast`. Nothing happens at runtime.
2. **Dynamic path** — otherwise `From` must be polymorphic, and:

   ```
   read the vptr of *p                      (one memcpy; first word of the object)
   if vptr == cached_vptr[thread][From,To]:
       if cached_offset == FAILED: return nullptr
       return (To)((char*)p + cached_offset)
   result = dynamic_cast<To>(p)             (slow path, once per vtable)
   cache vptr and (result - p) or FAILED
   return result
   ```

The cache key is the **vtable pointer of the `From` subobject**, which identifies both the
most-derived type *and* which subobject of it `p` points to. That is enough to make the offset
a constant: an object of a given dynamic type always has its `To` subobject at the same
distance from its `From` subobject. Two `Base` subobjects inside the same object have two
different vtables, so they get two different (correct) offsets.

Each `(From, To)` pair instantiates its own cache, and each cache is `thread_local`, so no
synchronisation is needed and there is no false sharing between threads. The cache holds one
entry: the last vtable seen.

## Caveats

- **Single-entry cache.** If one call site alternates between *different* dynamic types
  (`Circle`, `Square`, `Circle`, `Square`, …), every call misses and costs a `dynamic_cast`
  plus a few stores. It is never slower than `dynamic_cast` by more than that bookkeeping,
  but it is not faster either. See the [cold vs hot](#where-the-speedup-comes-from-the-cache)
  numbers.
- **RTTI is required.** The slow path is a real `dynamic_cast`; `-fno-rtti` / `/GR-` is
  not supported (it is not supported by `dynamic_cast` either).
- **Object layout assumptions.** The fast path reads the vptr from the first word of the
  object. This is guaranteed by the Itanium ABI and by MSVC, which covers every mainstream
  compiler and platform. On anything else `FASTCAST_SUPPORTED` is undefined and the header
  falls back to plain `dynamic_cast`.
- **Shared libraries** may end up with more than one copy of a vtable for the same type.
  That only causes an extra cache miss per copy; results stay correct because the key is the
  actual vtable address, not the type.
- **`volatile` pointees** are not supported on the dynamic path.

## Language and compiler support

The header requires **C++11**. Newer standards only add `constexpr`-ness:

| Standard | What you get |
|---|---|
| C++11 | Everything. The pointer overload is `constexpr` for static casts. |
| C++14 | The reference overload is also `constexpr` for static casts. |
| C++17 / C++20 | No changes. |
| C++23 | The dynamic path is declared `constexpr` too (usable in constant evaluation only for null pointers, since it uses `thread_local` storage). |

Every commit is built and tested by CI with warnings as errors on:

| Compiler | Standards | Extras |
|---|---|---|
| GCC (Ubuntu) | 11, 14, 17, 20, 23 | ASan + UBSan (incl. `-fsanitize=vptr`) at 11, 17, 23 |
| Clang (Ubuntu) | 11, 14, 17, 20, 23 | ASan + UBSan at 11, 17, 23 |
| AppleClang (macOS) | 11, 14, 17, 20, 23 | |
| MSVC (Windows, x64) | 14 (11 maps to 14), 17, 20, latest | |

plus an `install` + `find_package` job and a `clang-format` check.

## Benchmarks

Run on a 16-core 4.768 GHz CPU (mean of 5 repetitions, `-DCMAKE_BUILD_TYPE=Release`, GCC).
Reproduce with `./measure` and regenerate the plots with `benchmark/plot_results.py`
(see [the benchmark directory](benchmark/) and [Building and testing](#building-and-testing)).
CPU frequency scaling was enabled, so sub-nanosecond figures are throughput-limited and
should be read as "effectively free", not as precise latencies.

### Where the speedup comes from: the cache

`fast_cast` caches one `(vtable → offset)` entry per `(From, To)` type pair. The first
cast of a given dynamic type ("cold") falls through to a real `dynamic_cast` plus the
caching bookkeeping; every **subsequent** cast of that same type ("hot") is just a load
and a pointer adjustment. The honest comparison therefore has two regimes:

![cold vs hot](benchmark/plots/cold_vs_hot.png)

| `ComplexA* → ComplexB*` | fast_cast | dynamic_cast |
|-------------------------|-----------|--------------|
| **Cold** (cache miss, varying types) | 28.4 ns | 28.0 ns |
| **Hot** (cache hit, repeated type)   | 0.56 ns | 20.1 ns |

On a cold call `fast_cast` is **~as fast as (marginally slower than) `dynamic_cast`** —
it does the same `dynamic_cast` and then records the result. The win is entirely in the
hot path, where it is ~35–50× faster. If your workload casts wildly varying dynamic
types and never repeats, `fast_cast` is not faster than `dynamic_cast`.

### Hot-path latency (repeated casts of the same object)

Most real call sites cast the same handful of objects/types repeatedly, which keeps the
cache hot:

![per-call latency](benchmark/plots/per_call.png)

| Per call (hot)          | fast_cast | dynamic_cast |
|-------------------------|-----------|--------------|
| Pointer success         | 0.56 ns   | 4.11 ns      |
| Pointer failure         | 0.56 ns   | 8.49 ns      |
| Reused reference        | 0.56 ns   | 4.14 ns      |

Failure caching makes repeated *misses* just as cheap as hits.

### Throughput (2,000,000 reference casts, including object construction)

![throughput](benchmark/plots/throughput.png)

| 2,000,000 casts         | fast_cast | dynamic_cast |
|-------------------------|-----------|--------------|
| Simple hierarchy        | 2.28 ms   | 9.21 ms      |
| Complex hierarchy       | 2.43 ms   | 52.1 ms      |

The complex (virtual + multiple inheritance) hierarchy is where `dynamic_cast` is most
expensive and the cache pays off most.

### Static-safe path

`Derived* → Base*` is resolved at compile time, so `fast_cast`, `static_cast`, and
`dynamic_cast` all measure ~0.12 ns — identical and effectively free.

## Building and testing

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

CMake options (all default to `ON` when FastCast is the top-level project, `OFF` otherwise,
except the last two which default to `OFF`):

| Option | Effect |
|---|---|
| `FASTCAST_BUILD_TESTS` | Unit tests (Catch2 v2, vendored as `tests/catch.hpp`) and compile-failure tests. |
| `FASTCAST_BUILD_EXAMPLES` | Builds and runs `examples/basic.cpp`. |
| `FASTCAST_BUILD_BENCHMARKS` | Fetches Google Benchmark and builds the `measure` target. |
| `FASTCAST_INSTALL` | Generates `install` rules and the `FastCastConfig.cmake` package. |
| `FASTCAST_WARNINGS_AS_ERRORS` | `-Werror` / `/WX` for tests, examples and benchmarks. |
| `FASTCAST_ENABLE_SANITIZERS` | Builds tests and examples with ASan + UBSan. |
| `CMAKE_CXX_STANDARD` | Pick the standard to test under (`11` … `23`, default `23`). |

The test suite has three kinds of tests, selectable with `ctest -L <label>`:

- `unit` — the Catch2 suite in `tests/tests.cpp` (`./build/tests --list-tests` to enumerate;
  tags such as `[cache]`, `[threads]`, `[shared_ptr]` filter it, e.g. `./build/tests "[cache]"`).
- `compile_fail` — each file in `tests/compile_fail/` must *fail* to compile with a specific
  diagnostic (casting away `const`, non-polymorphic downcast, non-pointer target …).
- `example` — `examples/basic.cpp`, which is also the source of the README snippets.

To check the installed package works, see `tests/package/` and the `package` CI job.

### Running the benchmarks

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DFASTCAST_BUILD_BENCHMARKS=ON
cmake --build build --target measure
./build/measure --benchmark_filter='-.*threads:' --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true \
    --benchmark_format=json --benchmark_out=results.json
python3 benchmark/plot_results.py results.json benchmark/plots/
```

## Releasing

A release is a tag. Pushing `vX.Y.Z` runs [`release.yml`](.github/workflows/release.yml), which:

1. checks that the tag names the version in `fastcast.hpp` (and stops if not),
2. builds and tests, then produces the CPack tarballs (`package` job) and the NuGet package
   (`nuget` job, on Windows with `nuget pack contrib/nuget/FastCast.nuspec`),
3. creates the GitHub release with the `CHANGELOG.md` section for that version as notes
   (GitHub's generated notes appended) and all assets attached,
4. publishes the `.nupkg` to nuget.org by **trusted publishing** — the job's OIDC token is
   exchanged for a short-lived API key by `NuGet/login`; no long-lived secret is stored.

Releases are cut **automatically every Monday** by
[`weekly-release.yml`](.github/workflows/weekly-release.yml): if `main` has moved since the
last tag and the CMake workflow passed on it, CI runs `scripts/bump_version.py patch`
(header macros, the version test, and `CHANGELOG.md`'s *Unreleased* section become the new
version), commits, tags `vX.Y.Z`, and pushes with the release deploy key, which triggers
the pipeline above. A week without changes produces nothing. *Run workflow* does the same
on demand.

To cut one by hand, or for a minor/major bump:


```sh
scripts/bump_version.py minor        # or patch / major / an explicit X.Y.Z
git commit -am "Release $(scripts/bump_version.py --show)"
git tag -a "v$(scripts/bump_version.py --show)" -m "FastCast $(scripts/bump_version.py --show)"
git push --follow-tags origin main
```

Write the release notes into the *Unreleased* section of `CHANGELOG.md` as you go; the bump
moves them under the new version heading.

"Run workflow" on a branch builds the same artifacts and publishes nothing, which is how a
change to the pipeline is tried before a version rides on it.

### One-time nuget.org setup (trusted publishing)

1. On nuget.org, **Account → Trusted Publishing → Add**: repository owner `reach2sayan`,
   repository `FastCast`, workflow file `release.yml`, environment left blank. If the package
   does not exist yet the policy is created as *pending* and becomes permanent on first push.
2. In the GitHub repository, **Settings → Secrets and variables → Actions → Variables**: add
   `NUGET_USER` = your nuget.org profile name. The publish steps are skipped while it is unset,
   so forks and dry runs never try to push.
3. For the weekly release, a deploy key with write access, registered on the `main` ruleset's
   bypass list, with its private half in the `RELEASE_DEPLOY_KEY` secret:
   ```sh
   ssh-keygen -t ed25519 -N "" -C "fastcast release agent" -f fastcast_release_key
   gh api -X POST repos/reach2sayan/FastCast/keys -f title="release agent" \
       -f key="$(cat fastcast_release_key.pub)" -F read_only=false
   gh secret set RELEASE_DEPLOY_KEY < fastcast_release_key
   ```
   The key files are gitignored.

## Contributing

`main` is protected by a ruleset ([`.github/rulesets/main.json`](.github/rulesets/main.json)):
no direct pushes, no force-pushes, no deletion; changes land through a pull request whose
`ci` check (the aggregate of every CI job) is green and whose branch is up to date. Repository
admins are on the bypass list.

- Format with `clang-format` (LLVM style, see `.clang-format`); CI checks it.
- Keep the header and the tests C++11-clean; CI builds every standard from 11 to 23.
- The version is defined once, in the macros at the top of `fastcast.hpp`. CMake reads them,
  so bump those (and the test in `tests/tests.cpp`) and add a line to [`CHANGELOG.md`](CHANGELOG.md).
- Run the full local matrix before pushing if you can:
  `for s in 11 14 17 20 23; do cmake -S . -B b$s -DCMAKE_CXX_STANDARD=$s -DFASTCAST_WARNINGS_AS_ERRORS=ON && cmake --build b$s && ctest --test-dir b$s; done`

## License

MIT, see [LICENSE.txt](LICENSE.txt). The vendored `tests/catch.hpp` is Catch2 v2.13.10,
distributed under the Boost Software License 1.0.
