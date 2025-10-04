[![CMake](https://github.com/reach2sayan/Einstein_Summation/actions/workflows/action.yml/badge.svg)](https://github.com/reach2sayan/Einstein_Summation/actions/workflows/action.yml) [![C++](https://img.shields.io/badge/C++-%2300599C.svg?logo=c%2B%2B&logoColor=white)](#)

# FastCast

**`fast_cast`** is a modern C++ header-only library providing a **high-performance polymorphic cast** alternative to
`dynamic_cast`. It accelerates runtime type conversions in polymorphic hierarchies while maintaining safety and
correctness.

The code is heavily inspired by [FastDynamicCast](https://github.com/tobspr/FastDynamicCast). This is effectively a
modern C++ implementation of FastDynamicCast.

# Improvements from FastDynamicCast

The library also includes **compile-time optimizations**:

- If a cast can be resolved with `static_cast` or identity, it avoids any runtime overhead.
- Failed casts are cached to reduce repeated dynamic lookups.

---

## Features

- Pointer, reference, and `std::shared_ptr` casting.
- **Fast paths** for:
    - Exact types (identity cast)
    - `static_cast`-safe conversions
- **Dynamic path** with per-thread vtable offset caching.
- **Failed cast caching** for repeated misses.
- Header-only, lightweight, and requires only C++17+ (C++23 constexpr enhancements included).
- Works with complex inheritance, including multiple and virtual inheritance.

---

## Installation

Simply include the header in your project:

```cpp
#include "fastcast.hpp"
```

No linking is required.

## Usage

### Pointer Casting

```cpp
struct Base { virtual ~Base() = default; };
struct Derived : Base {};

Derived d;
Base* bp = &d;

Derived* dp1 = fast_cast<Derived*>(bp); // fast_dynamic_cast equivalent
```

### Reference Casting

```cpp
Derived d;
Base& br = d;

try {
    Derived& dr = fast_cast<Derived&>(br); // throws std::bad_cast on failure
} catch (const std::bad_cast& e) {
    std::cerr << "Invalid cast\n";
}
```

### Shared Pointer Casting

```cpp
auto sp_base = std::make_shared<Derived>();
auto sp_derived = fast_dynamic_pointer_cast<Derived>(sp_base);
```

### Identity Casting

```cpp
Derived* dp = &d;
auto same = fast_cast<Derived*>(dp); // trivial, no runtime overhead
```

Benchmarks

All benchmarks were run on a 16-core 4.768 GHz CPU.
Times are for 2,000,000 iterations unless otherwise specified.

| Benchmark                    | FastCast       | dynamic_cast | static_cast |
|------------------------------|----------------|--------------|-------------|
| Simple class hierarchy       | 2.01 ms        | 7.28 ms      | N/A         |
| Complex hierarchy            | 1.96–2.06 ms   | 44–46 ms     | N/A         |
| Pointer success              | 0.448 ns       | 2.96–3.03 ns | N/A         |
| Pointer failure              | 0.671 ns       | 6.84 ns      | N/A         |
| Reused pointer (cached)      | 0.457–0.579 ns | 3.02–3.88 ns | N/A         |
| Derived → Base (static path) | 0.110 ns       | 0.110 ns     | 0.109 ns    |

#### Observations:

- Static-safe casts are essentially free, matching static_cast.
- Dynamic path for first-time polymorphic casts is comparable to dynamic_cast.
- Repeated casts hit the cache and are orders of magnitude faster.
- Failure caching ensures repeated invalid casts are extremely cheap.

## Requirements

- C++17 minimum (C++23 optional for constexpr enhancements)
- Header-only, no linking required
- Optional: Boost (for tests), Google Benchmark (for benchmarks)