# Changelog

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/).

## [Unreleased]

## [1.1.0] - 2026-09-08

### Added
- **C++11 support.** The header now compiles from C++11 upward (previously C++20 was
  required because of `requires` clauses, `if constexpr` and the `_v`/`_t` trait aliases).
  Overload selection uses `std::enable_if`; the static/dynamic split uses tag dispatch.
- `fast_cast<void *>(p)` (and `const void *`) now returns the most-derived object, exactly
  like `dynamic_cast<void *>`. Previously it silently degraded to `static_cast<void *>`.
- Fallback to plain `dynamic_cast` on ABIs where the vtable-pointer trick is not known to
  hold (`FASTCAST_SUPPORTED` undefined), instead of defining nothing.
- `FASTCAST_CPLUSPLUS` (language version detection that also works on MSVC without
  `/Zc:__cplusplus`) and `FASTCAST_NO_GLOBAL_USING` (opt out of the global `using`s).
- `noexcept` on the pointer and `shared_ptr` overloads.
- The static path is `constexpr` in every supported standard; the reference overload from
  C++14; the dynamic path from C++23.
- CMake: `install()` rules and a `FastCastConfig.cmake` package (`find_package(FastCast)` →
  `FastCast::FastCast`), CPack tarball/zip packaging, `FASTCAST_BUILD_EXAMPLES`,
  `FASTCAST_INSTALL`, `FASTCAST_WARNINGS_AS_ERRORS` and `FASTCAST_ENABLE_SANITIZERS` options.
  The project version is read from the header macros.
- Tests: coverage for `shared_ptr`, `const`, `void *`, cross-casts with non-zero offsets,
  virtual bases, sibling subobjects of the same type, cache alternation between success and
  failure, cache churn across many types, casting during construction, a randomised parity
  test against `dynamic_cast`, multi-threaded correctness, compile-time type/`noexcept`/
  `constexpr` checks, and compile-failure tests for the `static_assert`s.
- `examples/basic.cpp`, built and run by `ctest`.
- CI: GCC, Clang, AppleClang and MSVC at C++11/14/17/20/23 with warnings as errors;
  ASan + UBSan jobs; an install + `find_package` job; a `clang-format` check; a tag-driven
  release workflow that attaches the packaged tarball, zip and single header.

### Changed
- `std::byte` replaced by `unsigned char` in the pointer arithmetic (C++11).
- Benchmark: duplicate benchmark registrations removed; `volatile` compound assignments
  (deprecated in C++20) rewritten.
- README rewritten: installation options, API reference, semantics vs `dynamic_cast`, how
  the cache works, caveats, support matrix, testing and release instructions.

## [1.0.0] - 2026-06-07

### Added
- Initial release: `fast_cast` for pointers and references, `fast_dynamic_pointer_cast`
  for `std::shared_ptr`, per-thread vtable/offset cache with failure caching, version
  macros, Catch2 tests and Google Benchmark benchmarks.

[Unreleased]: https://github.com/reach2sayan/FastCast/compare/v1.1.0...HEAD
[1.1.0]: https://github.com/reach2sayan/FastCast/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/reach2sayan/FastCast/releases/tag/v1.0.0
