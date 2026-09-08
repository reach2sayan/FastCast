// fastcast.hpp -- a cached, drop-in replacement for dynamic_cast.
//
// SPDX-License-Identifier: MIT
// Created by sayan on 10/4/25.
//
// Requires C++11 or later. See README.md for the design and the caveats.

#pragma once

#define FASTCAST_VERSION_MAJOR 1
#define FASTCAST_VERSION_MINOR 1
#define FASTCAST_VERSION_PATCH 0
#define FASTCAST_VERSION                                                       \
  (FASTCAST_VERSION_MAJOR * 10000 + FASTCAST_VERSION_MINOR * 100 +             \
   FASTCAST_VERSION_PATCH)
#define FASTCAST_VERSION_STRING "1.1.0"

#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <type_traits>
#include <typeinfo>

// ---------------------------------------------------------------------------
// Language-version detection.
//
// MSVC reports 199711L in __cplusplus unless /Zc:__cplusplus is given, but it
// always reports the real value in _MSVC_LANG.
// ---------------------------------------------------------------------------
#if defined(_MSVC_LANG) && _MSVC_LANG > __cplusplus
#define FASTCAST_CPLUSPLUS _MSVC_LANG
#else
#define FASTCAST_CPLUSPLUS __cplusplus
#endif

#if FASTCAST_CPLUSPLUS < 201103L
#error "fastcast.hpp requires C++11 or later"
#endif

// C++14 relaxed constexpr (if / multiple statements in a constexpr body).
#if FASTCAST_CPLUSPLUS >= 201402L
#define FASTCAST_CONSTEXPR14 constexpr
#else
#define FASTCAST_CONSTEXPR14
#endif

// C++23 allows thread_local variables inside constexpr functions (P2242) and
// drops the literal-type requirements on constexpr signatures (P2448).
#if FASTCAST_CPLUSPLUS >= 202302L
#define FASTCAST_CONSTEXPR23 constexpr
#else
#define FASTCAST_CONSTEXPR23
#endif

// ---------------------------------------------------------------------------
// ABI detection.
//
// The cached fast path reads the vtable pointer from the first pointer-sized
// word of a polymorphic object. That layout is guaranteed by the Itanium C++
// ABI (GCC, Clang, ICC -- i.e. Linux, macOS, BSD, Android, iOS, MinGW) and by
// the MSVC ABI. On any other ABI FASTCAST_SUPPORTED stays undefined and every
// cast simply forwards to dynamic_cast, so the API is usable everywhere.
// ---------------------------------------------------------------------------
#if defined(__GXX_ABI_VERSION) || defined(_MSC_VER)
#define FASTCAST_SUPPORTED 1
#endif

namespace fastcast {
namespace detail {

// A cast From* -> To can be done entirely at compile time when it is an
// identity, cv-adding, or derived-to-base pointer conversion. Casting to
// void* is deliberately excluded: dynamic_cast<void*> yields the address of
// the most-derived object, which static_cast cannot compute.
template <typename To, typename From>
struct use_static_path
    : std::integral_constant<
          bool,
          std::is_convertible<From *, To>::value &&
              !std::is_void<typename std::remove_pointer<To>::type>::value> {};

// Static path: no runtime work at all. constexpr since C++11 (single return).
template <typename To, typename From>
constexpr inline
    typename std::enable_if<use_static_path<To, From>::value, To>::type
    cast_impl(From *ptr) noexcept {
  return static_cast<To>(ptr);
}

// Dynamic path: dynamic_cast with a per-thread, per-(From, To) cache of the
// last vtable seen and the pointer adjustment it required.
template <typename To, typename From>
FASTCAST_CONSTEXPR23 inline
    typename std::enable_if<!use_static_path<To, From>::value, To>::type
    cast_impl(From *ptr) noexcept {
  static_assert(std::is_polymorphic<From>::value,
                "fast_cast: source type is not polymorphic");
  if (!ptr) {
    return nullptr;
  }
#if defined(FASTCAST_SUPPORTED)
  typedef const void *vtable_ptr;
  constexpr std::ptrdiff_t NO_OFFSET =
      std::numeric_limits<std::ptrdiff_t>::max();
  constexpr std::ptrdiff_t FAILED_OFFSET =
      std::numeric_limits<std::ptrdiff_t>::min();

  // One cache entry per (From, To) pair per thread.
  thread_local static std::ptrdiff_t offset = NO_OFFSET;
  thread_local static vtable_ptr cached_vtable = nullptr;

  // Read the vptr without violating strict aliasing: memcpy is the only
  // well-defined way to type-pun the object storage into a pointer.
  vtable_ptr this_vtable;
  std::memcpy(&this_vtable, static_cast<const void *>(ptr), sizeof this_vtable);

  if (cached_vtable == this_vtable) {
    if (offset == FAILED_OFFSET) {
      return nullptr; // cached miss
    }
    unsigned char *raw = reinterpret_cast<unsigned char *>(
        const_cast<typename std::remove_cv<From>::type *>(ptr));
    return reinterpret_cast<To>(raw + offset);
  }

  // Slow path: real dynamic_cast, then remember what it did for this vtable.
  To result = dynamic_cast<To>(ptr);
  cached_vtable = this_vtable;
  offset = result ? reinterpret_cast<const unsigned char *>(result) -
                        reinterpret_cast<const unsigned char *>(ptr)
                  : FAILED_OFFSET;
  return result;
#else
  return dynamic_cast<To>(ptr);
#endif
}

} // namespace detail

// Pointer overload: fast_cast<To*>(From*). Never throws; returns nullptr on
// failure exactly like dynamic_cast.
template <typename To, typename From>
constexpr inline typename std::enable_if<std::is_pointer<To>::value, To>::type
fast_cast(From *ptr) noexcept {
  static_assert(
      !(std::is_const<From>::value &&
        !std::is_const<typename std::remove_pointer<To>::type>::value),
      "fast_cast cannot cast away const from pointee");
  static_assert(
      !(std::is_volatile<From>::value &&
        !std::is_volatile<typename std::remove_pointer<To>::type>::value),
      "fast_cast cannot cast away volatile from pointee");
  return detail::cast_impl<To>(ptr);
}

// Reference overload: fast_cast<To&>(From&). Throws std::bad_cast on failure
// exactly like dynamic_cast.
template <typename To, typename From>
FASTCAST_CONSTEXPR14 inline
    typename std::enable_if<std::is_reference<To>::value, To>::type
    fast_cast(From &ref) {
  typedef
      typename std::add_pointer<typename std::remove_reference<To>::type>::type
          ToPtr;
  ToPtr casted = fast_cast<ToPtr>(&ref);
  if (casted) {
    return *casted;
  }
  throw std::bad_cast();
}

// shared_ptr overload: the counterpart of std::dynamic_pointer_cast. The
// result shares ownership with the argument; an empty shared_ptr is returned
// on failure.
template <typename To, typename From>
FASTCAST_CONSTEXPR23 inline std::shared_ptr<To>
fast_dynamic_pointer_cast(const std::shared_ptr<From> &ptr) noexcept {
  To *raw = fast_cast<To *>(ptr.get());
  if (raw) {
    return std::shared_ptr<To>(ptr, raw);
  }
  return std::shared_ptr<To>();
}

} // namespace fastcast

// Define FASTCAST_NO_GLOBAL_USING before including this header to keep
// fast_cast / fast_dynamic_pointer_cast out of the global namespace.
#if !defined(FASTCAST_NO_GLOBAL_USING)
using fastcast::fast_cast;
using fastcast::fast_dynamic_pointer_cast;
#endif

#undef FASTCAST_CONSTEXPR14
#undef FASTCAST_CONSTEXPR23
