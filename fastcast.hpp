//
// Created by sayan on 10/4/25.
//

#ifndef FASTCAST_FASTCAST_HPP
#define FASTCAST_FASTCAST_HPP
#ifndef FAST_DYNAMIC_CAST_H
#define FAST_DYNAMIC_CAST_H

#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>
#pragma once
#if __cplusplus > 202302L
#define CONSTEXPR constexpr
#else
#define CONSTEXPR
#endif


namespace fastcast {

// Core implementation for pointer types
template <typename To, typename From> CONSTEXPR inline To cast_impl(From *ptr) {
  using v_table_ptr = const uintptr_t *;
  if constexpr (std::is_same_v<std::remove_cv_t<From>,
                               std::remove_pointer_t<To>>) {
    return ptr;
  } else if constexpr (std::is_convertible_v<From *, To>) {
    return static_cast<To>(ptr);
  } else {
    static_assert(std::is_polymorphic_v<From>, "Type is not polymorphic!");
    if (!ptr)
      return nullptr;

    constexpr std::ptrdiff_t NO_OFFSET =
        std::numeric_limits<std::ptrdiff_t>::max();
    constexpr std::ptrdiff_t FAILED_OFFSET =
        std::numeric_limits<std::ptrdiff_t>::min();

    // thread-local cache for this (From, To) pair
    thread_local static std::ptrdiff_t offset = NO_OFFSET;
    thread_local static v_table_ptr cached_vtable = nullptr;

    auto this_vtable = *reinterpret_cast<v_table_ptr *>(
        const_cast<std::remove_cv_t<From> *>(ptr));

    if (cached_vtable == this_vtable) {
      if (offset == FAILED_OFFSET) {
        return nullptr; // fast-fail
      }
      auto new_ptr = reinterpret_cast<std::byte *>(
                         const_cast<std::remove_cv_t<From> *>(ptr)) +
                     offset;
      return reinterpret_cast<To>(new_ptr);
    }

    // slow path
    auto result = dynamic_cast<To>(ptr);
    cached_vtable = this_vtable;
    if (result) {
      offset = reinterpret_cast<const std::byte *>(result) -
               reinterpret_cast<const std::byte *>(ptr);
    } else {
      offset = FAILED_OFFSET;
    }
    return result;
  }
}

// Pointer overload
template <typename To, typename From>
  requires std::is_pointer_v<To>
constexpr inline To fast_cast(From *ptr) {
  using ToNonPtr = std::remove_pointer_t<To>;
  using ToPtr =
      std::conditional_t<std::is_const_v<From>, const ToNonPtr *, ToNonPtr *>;
  return cast_impl<ToPtr>(ptr);
}

// Reference overload
template <typename To, typename From>
  requires std::is_reference_v<To>
constexpr inline To fast_cast(From &ref) {
  using ToPtr = std::add_pointer_t<std::remove_reference_t<To>>;
  auto casted_ptr = fast_cast<ToPtr>(&ref);
  if (!casted_ptr)
    throw std::bad_cast{};
  return *casted_ptr;
}

// shared_ptr overload
template <typename To, typename From>
constexpr inline std::shared_ptr<To>
fast_dynamic_pointer_cast(const std::shared_ptr<From> &ptr) {
  return std::shared_ptr<To>(ptr, fast_cast<To *>(ptr.get()));
}

// Identity cast (same type)
template <typename To, typename From>
  requires std::is_same_v<std::remove_cv_t<To>, std::remove_cv_t<From>>
constexpr inline To fast_cast(To ptr) {
  return ptr;
}
} // namespace fastcast

using fastcast::fast_cast;
using fastcast::fast_dynamic_pointer_cast;

#endif // FAST_DYNAMIC_CAST_H
#endif // FASTCAST_FASTCAST_HPP
